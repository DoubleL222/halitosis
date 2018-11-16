import hlt
import copy
import hlt.player

class SimulatedGame:
    """
    The is a representation of the Game class
    """
    def __init__(self, game_to_copy):
        if isinstance(game_to_copy, hlt.Game):
            self.game_copy = copy.deepcopy(game_to_copy)

    def print_test(self):
        print("from class: " + str(self.test))

    def advance_game(self, commands):
        for command in commands:
            print("new command ", command)



"""
wat = 7
sim_game = SimulatedGame(wat)
print("Wat: "+str(wat))
sim_game.print_test()
"""