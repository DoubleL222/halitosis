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
from hlt.game_map import MapCell
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

        #logging.info(self.game_copy.game_map._cells)
        # Profiling
        end = timeit.default_timer()
        #bot.profiling.print_ms_message((end-start), "Deep copy took")


        #Profiling
        self.deep_copy_time_sum = end-start
        self.advance_game_time_sum = 0
        self.bot_time_sum = 0

    def get_next_ship_id(self):
        max_id = 0
        for player_id in self.game_copy.players:
            ships_dict = self.game_copy.players[player_id].get_ships_dict()
            if ships_dict:
                max_p_id = max(self.game_copy.players[player_id].get_ships_dict())
                if max_p_id > max_id:
                    max_id = max_p_id
        return max_id + 1

    def move(self, position, dx, dy):
        position.x += dx
        position.y += dy
        return position

    def clean_map(self):
        #logging.info(self.game_copy.game_map._cells)
        #print("LEN: "+str(len(self.game_copy.game_map._cells[0])))
        logging.info("x: " + str(len(self.game_copy.game_map._cells)) + "; y: " + str(len(self.game_copy.game_map._cells[0])))
        for i in range(len(self.game_copy.game_map._cells)):
            for j in range(len(self.game_copy.game_map._cells[i])):
                #logging.info("i: "+str(i)+"; j: "+str(j))
                self.game_copy.game_map._cells[i][j].occupied_this_round = False

        #for index, cell in self.game_copy.game_map._cells:
        #    self.game_copy.game_map._cells[index].occupied_this_round = False

    def advance_game(self, commands, player_id):
        # Profiling
        start = timeit.default_timer()

        player = self.game_copy.players[player_id]
        # logging.info("Commands: " + str(len(commands)))
        for command in commands:
            split_command = command.split(" ")
            # SPAWN COMMAND
            if split_command[0] == "g":
                if player.halite_amount >= constants.SHIP_COST:
                    new_id = self.get_next_ship_id()
                    new_ship = Ship(player.id, new_id, player.shipyard.position, 0)
                    # print("len before: " + str(len(self.game_copy.players[player_id].get_ships())))
                    player.add_ship(new_ship, new_id)
                    player.halite_amount -= constants.SHIP_COST
                    self.game_copy.players[player_id] = player
                    # print("len after: " + str(len(self.game_copy.players[player_id].get_ships())))
                    # player._update(len(player.get_ships())+1, len(player.get_dropoffs()), player.halite_amount)
                else:
                    logging.warning("Player "+str(player_id)+" wanted to build ship, but doesn't have enough halite")
            # MOVE COMMAND
            elif split_command[0] == "m":
                ship_id = int(split_command[1])
                if self.game_copy.players[player_id].has_ship(ship_id):
                    ship_to_move = self.game_copy.players[player_id].get_ships_dict()[ship_id]
                    current_cell = self.game_copy.game_map[ship_to_move.position]
                    ship_position = ship_to_move.position
                    ship_halite = ship_to_move.halite_amount

                    failed_to_move = False
                    # IF NOT STAY STILL
                    if split_command[2] != "o":
                        move_cost = int(round((1/constants.MOVE_COST_RATIO) * current_cell.halite_amount))
                        logging.warning("Move costs: "+str(move_cost)+ "; Move Cost Ratio: "+str(constants.MOVE_COST_RATIO) + "; Extract ratio: "+str(constants.EXTRACT_RATIO))
                        # IF HAVE ENOUGH TO MOVE
                        if ship_halite >= move_cost:
                            if not current_cell.occupied_this_round:
                                current_cell.ship = None
                                self.game_copy.game_map[ship_to_move.position].ship = None
                            ship_halite -= move_cost
                            if split_command[2] == "n":
                                ship_position = self.move(ship_position, 1, 0)
                            elif split_command[2] == "s":
                                ship_position = self.move(ship_position, -1, 0)
                            elif split_command[2] == "e":
                                ship_position = self.move(ship_position, 0, 1)
                            elif split_command[2] == "w":
                                ship_position = self.move(ship_position, 0, -1)

                            ship_to_move.position = ship_position
                            ship_to_move.halite_amount = ship_halite

                            next_cell = self.game_copy.game_map[ship_to_move.position]
                            # Check for collision
                            if next_cell.occupied_this_round:
                                # Destroy both ships
                                logging.warning("Ship "+str(ship_id) + ", owned by " + str(
                                    player_id) + "Collided with "+ str(next_cell.ship.id) + ", owned by " + str(
                                    next_cell.ship.owner))
                                self.game_copy.players[player_id].remove_ship(ship_id)
                                #if next_cell.ship is not None:
                                #    self.game_copy.players[next_cell.ship.owner].remove_ship(next_cell.ship.id)
                                #    next_cell.ship = None
                                #    self.game_copy.game_map[ship_to_move.position].ship = None
                            else:
                                next_cell.ship = ship_to_move
                            next_cell.occupied_this_round = True
                            self.game_copy.game_map[ship_to_move.position].occupied_this_round = True
                            #self.game_copy.game_map[ship_to_move.position] = next_cell


                        else:
                            failed_to_move = True
                            logging.warning(str(ship_id) +" has "+ str(ship_to_move.halite_amount))
                            logging.warning(
                                "Player " + str(player_id) + " wanted to move ship " + str(
                                    ship_id) + ", but doesn't have enough halite")
                    if split_command[2] == "o" or failed_to_move:
                        gather_amount = int(round((1/constants.EXTRACT_RATIO) * current_cell.halite_amount))
                        ship_halite += gather_amount
                        ship_to_move.halite_amount = ship_halite
                        current_cell.halite_amount -= gather_amount
                        self.game_copy.game_map[ship_to_move.position].halite_amount = current_cell.halite_amount
                        self.game_copy.players[player_id].add_ship(ship_to_move, ship_to_move.id)
                else:
                    logging.warning(
                        "Player " + str(player_id) + " wanted to move ship "+str(ship_id)+", but doesn't have it")
                    logging.warning(str(self.game_copy.players[player_id].get_ships_dict()))
                #print("move")

            elif split_command[0] == "c":
                # TODO implement consturc dropoff
                print("Make dropoff")

        # Profiling
        end = timeit.default_timer()
        # bot.profiling.print_ms_message((end-start), "Advance game took")
        # TODO clean up map
        self.clean_map()
        self.advance_game_time_sum = self.advance_game_time_sum + (end-start)
    def run_simulation(self, bot):
        while self.game_copy.turn_number < 500:
            # print("Turn " + str(self.game_copy.turn_number))
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