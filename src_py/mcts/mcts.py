# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

import math
import copy
import random
import logging
#from graphviz import Digraph


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

    debug = True

    # exploration_constant note: Larger values will increase exploitation, smaller will increase exploration.
    def __init__(self, exploration_constant=1 / math.sqrt(2), game_state=None, ship=None,
                 current_turn=1, game_max_turns=1, do_merged_simulations=True,
                 use_best_action_list_for_other_ships=True, simulator=None, default_policy=None):
        self.explorationConstant = exploration_constant
        self.rootNode = TreeNode()
        self.ship = ship
        self.shipId = ship.id
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
        while self.doMergedSimulations or not node.isTerminal:
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
        if self.debug and not node.depth == 0:
            logging.info("Ship " + str(self.shipId) + " - Expanding node -> depth: " + str(node.depth) + ", action: " + node.action)

        # We always expand all possible nodes of a given node immediately.
        for action in Mcts.ship_commands:
            new_node = self.generate_new_node(node, action)
            if not self.doMergedSimulations:
                rewards = self.do_simulation(simulator=self.simulator, default_policy=self.defaultPolicy,
                                             ship_action_lists=self.compile_new_action_lists_for_individual_run(new_node))
                new_node.totalReward = rewards.pop(self.shipId, 0)
                if self.debug:
                    logging.info("Ship " + str(self.shipId) + " -> New node reward: " + str(new_node.totalReward))
                self.backpropagate(new_node, new_node.totalReward)
            else:
                self.lastGeneratedChildren[action] = new_node
            node.children.append(new_node)

        self.lastExpandedNode = node
        node.isFullyExpanded = True

        # Update the current best action list in the main dictionary, but only after simulating and
        # backpropagating all the new child nodes.
        if not self.doMergedSimulations:
            self.update_ship_best_action_list()
        return

    # Create a new action list, with the given action as the first action, and using the given parent's action as
    # the previous action, and move up the tree from that parent, adding actions on the way.
    def get_specific_action_list(self, bottom_action, parent):
        new_ship_action_tree = [self.ship.move(bottom_action)]

        if parent is not None:
            # Make a temp-variable, for walking up the tree of parents.
            parent_temp = parent

            # For each parent, insert their action first in the list, and set parent_temp to their parent, if any.
            while True:
                if parent_temp.action is None:
                    break
                new_ship_action_tree.insert(0, self.ship.move(parent_temp.action))
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
        if self.debug:
            if parent.depth == 0:
                logging.info("Ship " + str(self.shipId) + " -> Generating node -> depth " + str(parent.depth+1) + ", action: " + action +
                             " || parent -> root")
            else:
                logging.info("Ship " + str(self.shipId) + " -> Generating node -> depth " + str(parent.depth+1) + ", action: " + action +
                             " || parent -> depth: " + str(parent.depth) + ", action: " + parent.action)
        return TreeNode(is_terminal=self.currentTurn + parent.depth + 1 >= self.gameMaxTurns, depth=parent.depth+1,
                        parent=parent, action=action, initial_reward=0)

    def compile_new_action_lists_for_individual_run(self, node):
        if self.useBestActionListForOtherShips:
            # Copy the current best action lists for all ships.
            temp_ship_best_action_lists = copy.deepcopy(self.ship_best_action_lists)
        else:
            temp_ship_best_action_lists = {}

        # Create a new action list for this ship, with the given action as the first action.
        if self.debug:
            logging.info("Ship " + str(self.shipId) + " -> node.action: " + str(node.action))
        new_ship_action_tree = self.get_specific_action_list(node.action, node.parent)
        if self.debug:
            logging.info("Ship " + str(self.shipId) + " -> new_ship_action_tree: " + str(new_ship_action_tree))

        # Replace our ship's current action list (in the copy) with our new action list.
        temp_ship_best_action_lists[self.shipId] = new_ship_action_tree
        if self.debug:
            logging.info("Ship " + str(self.shipId) + " -> temp_ship_best_action_lists: " + str(temp_ship_best_action_lists))

        return temp_ship_best_action_lists

    @staticmethod
    def do_simulation(simulator=None, default_policy=None, ship_action_lists=None):
        if simulator is None:
            rewards = {}
            for ship_id, action_list in ship_action_lists.items():
                rewards[ship_id] = random.randrange(1, 10)
        else:
            # Do the simulation, using our current game state, and our new action list,
            # as well as the current best actions for the other ships.
            # Call Luka's simulation thingy with the state and the given ship_action_trees,
            # to receive a reward.
            rewards = simulator.run_simulation(default_policy, ship_action_lists)
        return rewards

    def best_child(self, node):
        best_score = -1.0
        best_children = []
        for child in node.children:
            exploit = child.totalReward / child.visits
            explore = math.sqrt(2.0 * math.log(node.visits) / float(child.visits))
            score = exploit + self.explorationConstant * explore
            if score > best_score:
                best_children = [child]
                best_score = score
            if math.isclose(a=score, b=best_score):
                best_children.append(child)
        if len(best_children) == 0:
            return random.choice(node.children)
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

    # def generate_graph(self):
    #     g = Digraph('G', filename='mcts_graph' + str(random.randint(0, 10000)) + '.gv')
    #     g.node("root", label="Root")
    #     self.recursive_graph_node(g, self.rootNode, "root", "r")
    #     g.view()
    #
    # def recursive_graph_node(self, g, node, parent_name, parent_action):
    #     for child in node.children:
    #         node_name = str(child.depth - 1) + parent_action + "->" + str(child.depth) + child.action
    #         node_label = "a: " + child.action + ", s:" + "{:.2f}".format(child.totalReward / child.visits) + ", v:" + str(child.visits)
    #         g.node(node_name, label=node_label)
    #         g.edge(parent_name, node_name)
    #         self.recursive_graph_node(g, child, node_name, child.action)
