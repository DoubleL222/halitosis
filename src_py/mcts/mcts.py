# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

import math
import copy
import random


class TreeNode:

    def __init__(self, is_terminal=False, depth=0, parent=None, action=None, initial_reward=0):
        self.isTerminal = is_terminal
        self.depth = depth
        self.parent = parent
        self.action = action
        self.totalReward = initial_reward

        self.isFullyExpanded = self.isTerminal
        self.visits = 0
        self.children = []


class Mcts:
    ship_commands = {'n', 's', 'e', 'w', 'o'}
    ship_best_action_lists = {}

    # exploration_constant note: Larger values will increase exploitation, smaller will increase exploration.
    def __init__(self, exploration_constant=1 / math.sqrt(2), game_state=None, ship_id=None,
                 current_turn=1, game_max_turns=1, do_merged_simulations=True,
                 use_best_action_list_for_other_ships=True, simulator=None, default_policy=None):
        self.explorationConstant = exploration_constant
        self.rootNode = TreeNode()
        self.shipId = ship_id
        self.currentTurn = current_turn
        self.gameMaxTurns = game_max_turns
        self.gameState = game_state
        self.doMergedSimulations = do_merged_simulations
        self.useBestActionListForOtherShips = use_best_action_list_for_other_ships
        self.simulator = simulator
        self.defaultPolicy = default_policy
        self.lastExpandedNode = None
        self.lastGeneratedChildren = {}

    # function UCTSEARCH(s0)
    #     create root node v0 with state s0
    #     while within computational budget do
    #         vl ← TREEPOLICY(v0)
    #         ∆ ← DEFAULTPOLICY(s(vl))
    #         BACKUP(vl, ∆)
    #     return a(BESTCHILD(v0, 0))

    def do_one_uct_update(self):
        self.tree_policy()
        return

    # function TREEPOLICY(v)
    #     while v is nonterminal do
    #         if v not fully expanded then
    #             return EXPAND(v)
    #         else
    #             v ← BESTCHILD(v, Cp)
    #     return v

    # NOTE: Our tree_policy doesn't return anything, since everything happens during expansion, and we just want to
    # continue expanding as much as we can. Each expansion expands all children of some node, simulates them all,
    # and updates the current best action list for the ship. If it reaches a terminal node, only the backpropagation
    # happens.
    def tree_policy(self):
        node = self.rootNode
        while not self.doMergedSimulations or not node.isTerminal:
            if not node.isFullyExpanded:
                self.expand(node)
                return
            else:
                node = self.best_child(node)

    def get_current_best_action_node(self):
        node = self.rootNode
        while not node.isTerminal and node.isFullyExpanded:
            node = self.best_child_by_reward_only(node)
        return node

    # function EXPAND(v)
    #     choose a ∈ untried actions from A(s(v))
    #     add a new child v' to v
    #         with s(v') = f(s(v), a)
    #         and a(v') = a
    #     return v'

    def expand(self, node):
        # We always expand all possible nodes of a given node immediately.
        while not node.isFullyExpanded:
            for action in Mcts.ship_commands:
                new_node = self.generate_new_node(node, action)
                if not self.doMergedSimulations:
                    rewards = self.do_simulation(default_policy=self.defaultPolicy, simulator=self.simulator,
                                                 ship_action_lists=self.compile_new_action_lists_for_individual_run(node))
                    new_node.totalReward = rewards.pop(self.shipId, 0)
                    self.backpropagate(new_node, new_node.totalReward)
                else:
                    self.lastGeneratedChildren[action] = new_node
                node.children.append(new_node)
                self.lastExpandedNode = node

        # Update the current best action list in the main dictionary, but only after simulating and
        # backpropagating all the new child nodes.
        if self.doMergedSimulations:
            self.update_ship_best_action_list()
        return

    # Create a new action list, with the given action as the first action, and using the given parent's action as
    # the previous action, and move up the tree from that parent, adding actions on the way.
    def get_specific_action_list(self, bottom_action, parent):
        new_ship_action_tree = [bottom_action]

        if parent is not None:
            # Make a temp-variable, for walking up the tree of parents.
            parent_temp = parent

            # For each parent, insert their action first in the list, and set parent_temp to their parent, if any.
            while True:
                if parent_temp.action is None:
                    break
                new_ship_action_tree.insert(0, parent_temp.action)
                if parent_temp.parent is None:
                    break
                parent_temp = parent_temp.parent

        return new_ship_action_tree

    def update_ship_best_action_list(self):
        best_node = self.get_current_best_action_node()
        new_ship_action_tree = self.get_specific_action_list(best_node.action, best_node.parent)
        self.ship_best_action_lists[self.shipId] = new_ship_action_tree
        return

    def generate_new_node(self, parent, action):
        # Create a new tree node with 0 reward.
        return TreeNode(is_terminal=self.currentTurn + parent.depth + 1 >= self.gameMaxTurns, depth=parent.depth+1,
                        parent=parent, action=action, initial_reward=0)

    def compile_new_action_lists_for_individual_run(self, node):
        if self.useBestActionListForOtherShips:
            # Copy the current best action lists for all ships.
            temp_ship_best_action_lists = copy.deepcopy(self.ship_best_action_lists)
        else:
            temp_ship_best_action_lists = {}

        # Create a new action list for this ship, with the given action as the first action.
        new_ship_action_tree = self.get_specific_action_list(node.action, node.parent)

        # Replace our ship's current action list (in the copy) with our new action list.
        temp_ship_best_action_lists[self.shipId] = new_ship_action_tree

    @staticmethod
    def do_simulation(simulator, default_policy, ship_action_lists):

        # Do the simulation, using our current game state, and our new action list,
        # as well as the current best actions for the other ships.
        # Call Luka's simulation thingy with the state and the given ship_action_trees,
        # to receive a reward.
        rewards = simulator.run_simulation(default_policy, ship_action_lists)

        # return self.gameState.function_to_call(ship_action_lists)
        return rewards
        # return random.randrange(1, 10)

    def best_child(self, node):
        best_score = 0.0
        best_children = []
        for child in node.children:
            exploit = child.totalReward / child.visits
            explore = math.sqrt(2.0 * math.log(node.visits) / float(child.visits))
            score = exploit + self.explorationConstant * explore
            if score == best_score:
                best_children.append(child)
            if score > best_score:
                best_children = [child]
                best_score = score
        return random.choice(best_children)

    # Used only to find the current best action, to make an action list from.
    def best_child_by_reward_only(self, node):
        best_score = 0.0
        best_children = []
        for child in node.children:
            score = child.totalReward / child.visits
            if score == best_score:
                best_children.append(child)
            if score > best_score:
                best_children = [child]
                best_score = score
        return random.choice(best_children)

    # function BESTCHILD(v, c)
    # return (big calculation)
    #
    # function DEFAULTPOLICY(s)
    #     while s is non-terminal do
    #         choose a ∈ A(s) uniformly at random
    #         s ← f(s, a)
    #     return reward for state s
    #
    # function BACKUP(v, ∆)
    #     while v is not null do
    #         N(v) ← N(v) + 1
    #         Q(v) ← Q(v) + ∆(v, p)
    #         v ← parent of v

    def backpropagate(self, node, reward):
        while node is not None:
            node.visits += 1
            node.totalReward += reward
            node = node.parent
        return
