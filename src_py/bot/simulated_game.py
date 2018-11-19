import hlt
import copy
import hlt.player
# This library contains constant values.
from hlt import constants
# This library allows you to generate random numbers.
import random
# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction
#Import ship
from hlt.entity import Ship
#import logging
import logging

import timeit

import bot.profiling

class GameSimulator:
    """
    The is a representation of the Game class
    """
    def __init__(self, game_to_copy):
        # Profiling
        start = timeit.default_timer()

        if isinstance(game_to_copy, hlt.Game):
            self.game_copy = copy.deepcopy(game_to_copy)
            self.game_copy.players = copy.deepcopy(game_to_copy.players)

        # Profiling
        end = timeit.default_timer()
        #bot.profiling.print_ms_message((end-start), "Deep copy took")

        self.deep_copy_time_sum = end-start
        self.advance_game_time_sum = 0
        self.bot_time_sum = 0

    def print_test(self):
        print("from class: " + str(self.test))

    def advance_game(self, commands, player_id):
        # Profiling
        start = timeit.default_timer()

        player = self.game_copy.players[player_id]
        #logging.info("Commands: " + str(len(commands)))
        for command in commands:
            if command == "g":
                #logging.info("Spawn ship")
                # GET MAX ID
                max_id = 0
                for player_id in self.game_copy.players:
                    for ship in self.game_copy.players[player_id].get_ships():
                        if ship.id > max_id:
                            max_id = ship.id
                max_id = max_id + 1
                new_ship = Ship(player.id, max_id , player.shipyard.position, 0)
                #print("len before: " + str(len(self.game_copy.players[player_id].get_ships())))
                player._ships[max_id] = new_ship
                self.game_copy.players[player_id] = player
                #print("len after: " + str(len(self.game_copy.players[player_id].get_ships())))
                #player._update(len(player.get_ships())+1, len(player.get_dropoffs()), player.halite_amount)

        # Profiling
        end = timeit.default_timer()
        #bot.profiling.print_ms_message((end-start), "Advance game took")
        self.advance_game_time_sum = self.advance_game_time_sum + (end-start)

    def run_simulation(self, bot):
        while self.game_copy.turn_number < 500:
            #print("Turn " + str(self.game_copy.turn_number))
            logging.info("+++++++++ TURN {:03} +++++++++ :SIM".format(self.game_copy.turn_number))
            # Profiling
            start = timeit.default_timer()
            returns = bot.run(self.game_copy)
            # Profiling
            end = timeit.default_timer()
            self.bot_time_sum = self.bot_time_sum + (end - start)

            self.advance_game(returns[0], returns[1])
            self.game_copy.turn_number = self.game_copy.turn_number+1


"""
class SimGame:
    def __init__(self, game):
        self.turn_number = game.turn_number
        self.players = {}
        for player_id in game.players:
            self.players[player_id] = SimPlayer(game.players[player_id])

class SimPlayer:
    def __init__(self, player):
        self.id = player.id
        self.shipyard =

class SimEntity:
    def __init__(self, owner, id, position):
        self.owner = owner
        self.id = id
        self.position = position

    def __repr__(self):
        return "{}(id={}, {})".format(self.__class__.__name__,
                                      self.id,
                                      self.position)

"""
"""
wat = 7
sim_game = SimulatedGame(wat)
print("Wat: "+str(wat))
sim_game.print_test()
"""