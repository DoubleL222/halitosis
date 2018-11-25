# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

import math
import copy
import random
import logging


class TreeNode:
    ship_commands = {'n', 's', 'e', 'w', 'o'}

    def __init__(self, is_terminal=False, depth=0, parent=None, action=None, initial_reward=0):
        self.isTerminal = is_terminal
        self.depth = depth
        self.parent = parent
        self.action = action
        self.totalReward = initial_reward

        self.isFullyExpanded = self.isTerminal
        self.visits = 0
        self.children = []
        self.untried_actions = copy.deepcopy(self.ship_commands)

    def get_untried_action(self):
        if self.untried_actions.__len__() == 1:
            self.isFullyExpanded = True
        return self.untried_actions.pop()


class Mcts:
    ship_best_action_lists = {}

    # exploration_constant note: Larger values will increase exploitation, smaller will increase exploration.
    def __init__(self, exploration_constant=1 / math.sqrt(2), game_state=None, ship_id=None,
                 current_turn=1, game_max_turns=1):
        self.explorationConstant = exploration_constant
        self.rootNode = TreeNode()
        self.shipId = ship_id
        self.currentTurn = current_turn
        self.gameMaxTurns = game_max_turns
        self.gameState = game_state

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
        while not node.isTerminal:
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
        # We always expand all possible nodes of a given node immediately, and do backpropagation on all of them.
        while not node.isFullyExpanded:
            action = node.get_untried_action()
            new_node = self.generate_new_node(node, action)
            node.children.append(new_node)
            self.backpropagate(new_node, new_node.totalReward)

        # Update the current best action list in the main dictionary.
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
        # Copy the current best action lists for all ships.
        temp_ship_best_action_lists = copy.deepcopy(self.ship_best_action_lists)

        # Create a new action list for this ship, with the given action as the first action.
        new_ship_action_tree = self.get_specific_action_list(action, parent)

        # Replace our ship's current action list (in the copy) with our new action list.
        temp_ship_best_action_lists[self.shipId] = new_ship_action_tree

        # Do the simulation, using our current game state, and our new action list,
        # as well as the current best actions for the other ships.
        total_reward = self.do_simulation(temp_ship_best_action_lists)

        # Create a new tree node.
        return TreeNode(is_terminal=self.currentTurn + parent.depth + 1 >= self.gameMaxTurns, depth=parent.depth+1,
                        parent=parent, action=action, initial_reward=total_reward)

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

    def do_simulation(self, ship_action_trees):
        # Call Luka's simulation thingy with the state and the given ship_action_trees,
        # to receive a new state and a reward.

        # return self.gameState.function_to_call(ship_action_trees)

        return random.randrange(1, 10)
