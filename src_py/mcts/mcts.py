# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

import math
import time

ship_commands = {'n', 's', 'e', 'w', 'o', 'c'}


class TreeNode:

    def __init__(self, state, parent):
        self.state = state
        self.isTerminal = state.isTerminal()
        self.isFullyExpanded = self.isTerminal
        self.parent = parent
        self.numVisits = 0
        self.totalReward = 0
        self.children = {}
        self.untried_actions = {}

        for command in ship_commands:
            self.untried_actions.add(str(command))

    def get_untried_action(self):
        if self.untried_actions.__len__() == 1:
            self.isFullyExpanded = True
        return self.untried_actions.pop()


class Mcts:



    def default_policy_fn(self, state):
        return state.getReward()

    def __init__(self, time_limit=None, iteration_limit=None, exploration_constant=1 / math.sqrt(2),
                 default_policy=default_policy_fn):
        if time_limit is not None:
            if iteration_limit is not None:
                raise ValueError("Cannot have both a time limit and an iteration limit")
            # time taken for each MCTS search in milliseconds
            self.timeLimit = time_limit
            self.limitType = 'time'
        else:
            if iteration_limit is None:
                raise ValueError("Must have either a time limit or an iteration limit")
            # number of iterations of the search
            if iteration_limit < 1:
                raise ValueError("Iteration limit must be greater than one")
            self.searchLimit = iteration_limit
            self.limitType = 'iterations'
            self.explorationConstant = exploration_constant
            self.defaultPolicy = default_policy
            self.rootNode = None

    # function UCTSEARCH(s0)
    #     create root node v0 with state s0
    #     while within computational budget do
    #         vl ← TREEPOLICY(v0)
    #         ∆ ← DEFAULTPOLICY(s(vl))
    #         BACKUP(vl, ∆)
    #     return a(BESTCHILD(v0, 0))

    def start_uct_search(self, initialState):
        # Create root node with initialState
        self.rootNode = TreeNode(initialState, None)
        if self.limitType == 'time':
            time_limit = time.time() + self.timeLimit / 1000
            while time.time() < time_limit:
                self.do_uct_search()
        else:
            for i in range(self.searchLimit):
                self.do_uct_search()

        best_child = self.getBestChild(self.root, 0)
        return self.getAction(self.root, best_child)

    def do_uct_search(self):
        node = self.selectNode(self.root)
        reward = self.defaultPolicy(node.state)
        self.backpropogate(node, reward)

    # function TREEPOLICY(v)
    #     while v is nonterminal do
    #         if v not fully expanded then
    #             return EXPAND(v)
    #         else
    #             v ← BESTCHILD(v, Cp)
    #     return v

    # function EXPAND(v)
    #     choose a ∈ untried actions from A(s(v))
    #     add a new child v' to v
    #         with s(v') = f(s(v), a)
    #         and a(v') = a
    #     return v'


    def expand(self, node):
        #node.state.get_expansion_states
        action = node.get_untried_action()
        node.children.add(self.generate_new_node(node, action))



    def generate_new_node(self, node, action):
        player = node.state.players[node.state.my_id]
        ships = player.get_ships()

        for ship in ships:
            for command in ship_commands:
                self.untried_actions.add(str(ship.id) + "||" + str(command))
        if action == 'g':
            # Our action is to generate a ship
            node.state.
        else:

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
