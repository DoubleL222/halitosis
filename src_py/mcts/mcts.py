# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction


def defaultPolicyFn(state):
    return state.getReward()

class TreeNode:
    def __init__(self, state, parent):
        self.state = state
        self.isTerminal = state.isTerminal()
        self.isFullyExpanded = self.isTerminal
        self.parent = parent
        self.numVisits = 0
        self.totalReward = 0
        self.children = {}

class Mcts:
    def __init__(self, timeLimit=None, iterationLimit=None, explorationConstant=1 / math.sqrt(2),
                 defaultPolicy=defaultPolicyFn):
        if timeLimit is not None:
            if iterationLimit is not None:
                raise ValueError("Cannot have both a time limit and an iteration limit")
            # time taken for each MCTS search in milliseconds
            self.timeLimit = timeLimit
            self.limitType = 'time'
        else:
            if iterationLimit is None:
                raise ValueError("Must have either a time limit or an iteration limit")
            # number of iterations of the search
            if iterationLimit < 1:
                raise ValueError("Iteration limit must be greater than one")
            self.searchLimit = iterationLimit
            self.limitType = 'iterations'
            self.explorationConstant = explorationConstant
            self.defaultPolicy = defaultPolicy

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
            timeLimit = time.time() + self.timeLimit / 1000
            while time.time() < timeLimit:
                self.do_uct_search()
        else:
            for i in range(self.searchLimit):
                self.do_uct_search()

        bestChild = self.getBestChild(self.root, 0)
        return self.getAction(self.root, bestChild)

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
        node.state.get_expansion_states

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

    def get_possible_actions(self, node, state):
        player = state.players[state.my_id]
        ships = player.get_ships()
        for ship in ships:
            for dir in Direction.
            node[str(ship.id) + str()]