import hlt
import copy
import hlt.player
# This library contains constant values.
from hlt import constants
# This library allows you to generate random numbers.
import random
# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction
#import ship
from hlt.entity import Ship
from hlt.positionals import Position
from hlt.game_map import MapCell
#import logging
import logging
import bot.profiling

import timeit

import bot.profiling



class GameSimulator:
    """
    The is a representation of the Game class
    """
    def __init__(self, game_to_copy, to_search_depth=-1):
        # Should I debug that
        self.do_debug = True
        self.do_performance_debug = False
        self.do_warnings = False
        self.generate_new_ships = False

        if self.do_performance_debug:
            # Profiling
            start = timeit.default_timer()

        if to_search_depth == -1:
            to_search_depth = hlt.constants.MAX_TURNS
        # Set search depth for simulation
        self.search_depth = to_search_depth

        if isinstance(game_to_copy, hlt.Game):
            self.original_game = game_to_copy
        #    self.original_game = copy.deepcopy(game_to_copy)
        #    self.original_game.players = copy.deepcopy(game_to_copy.players)

        # Setting shipyard position cells halite to 0
        for curr_player_id in self.original_game.players:
            curr_player = self.original_game.players[curr_player_id]
            self.original_game.game_map.set_cell_halite(curr_player.shipyard.position, 0)

        # Init game copy
        self.game_copy = None

        # Dictionary for keeping ship scores for Monte Carlo
        self.ship_scores = {}

        if self.do_performance_debug:
            # Profiling
            end = timeit.default_timer()
            # Profiling sums
            self.deep_copy_time_sum = end-start
            self.advance_game_time_sum = 0
            self.bot_time_sum = 0
            self.map_cleanup_sum = 0
            self.executing_moves_sum = 0

        # Cells to clean at the end of advance game
        self.cells_to_clean = []

    def reset_simulator(self):
        self.game_copy = copy.deepcopy(self.original_game)
        self.game_copy.players = copy.deepcopy(self.original_game.players)
        if self.do_performance_debug:
            self.advance_game_time_sum = 0
            self.bot_time_sum = 0
            self.map_cleanup_sum = 0
            self.executing_moves_sum = 0
        self.cells_to_clean = []
        self.init_ship_scores()

    def init_ship_scores(self):
        # Dictionary for keeping ship scores for Monte Carlo
        self.ship_scores = {}

        # Setting shipyard position cells halite to 0
        for curr_player_id in self.game_copy.players:
            curr_player = self.game_copy.players[curr_player_id]
            # Create ship dictionary for keeping score
            for current_ship in curr_player.get_ships():
                self.ship_scores[current_ship.id] = 0

    def add_halite_score_to_ship(self, ship_id, halite):
        if ship_id in self.ship_scores:
            self.ship_scores[ship_id] += halite

    def print_map(self):
        #Iterate through all cells and print contents
        print_string = "\n"
        for y in range(self.game_copy.game_map.width):
            for x in range(self.game_copy.game_map.height):
                pos = Position(x, y)
                map_cell = self.game_copy.game_map[pos]
                print_string += "h: " + str(map_cell.halite_amount)
                if map_cell.ship is not None:
                    print_string += ", s(%d): %d " % (map_cell.ship.id ,map_cell.ship.halite_amount)
                print_string += " || "
            print_string += "\n"
        logging.info(print_string)

    def get_next_ship_id(self):
        # Get id for newly spawned ship
        max_id = 0
        for player_id in self.game_copy.players:
            ships_dict = self.game_copy.players[player_id].get_ships_dict()
            if ships_dict:
                max_p_id = max(self.game_copy.players[player_id].get_ships_dict())
                if max_p_id > max_id:
                    max_id = max_p_id
        return max_id + 1

    def move(self, position, dx, dy):
        # Get new position based on current position and move direction
        position.x += dx
        position.y -= dy
        return self.game_copy.game_map.normalize(position)

    def clean_map(self):
        # Reset necessary map cells members
        # for i in range(len(self.game_copy.game_map.get_cells())):
        #    for j in range(len(self.game_copy.game_map.get_cells()[i])):
        for position in self.cells_to_clean:
                #current_cell = self.game_copy.game_map.get_cells()[i][j]
                current_cell = self.game_copy.game_map[position]
                ship_queue = self.game_copy.game_map.get_cell_ship_queue(current_cell.position)
                # If only one ship entering/staying in cell put it there
                if len(ship_queue) == 1:
                    self.game_copy.game_map.add_ship_to_cell(current_cell.position, ship_queue[0])
                # If more than one ship coming to same cell, destroy all
                elif len(ship_queue) > 1:
                    if self.do_warnings:
                        logging.warning("Ships collided on map cell: i: " + str(current_cell.position.x) + ", j: " + str(current_cell.position.y) + "; ships:")
                    for current_ship in ship_queue:
                        if self.do_warnings:
                            logging.warning("Ship id: " + str(current_ship.id) + ", owner: " + str(current_ship.owner))
                        self.game_copy.players[current_ship.owner].remove_ship(current_ship.id)
                # Clear ship queue for cell
                self.game_copy.game_map.clear_ship_queue(current_cell.position)

        self.cells_to_clean = []
        #self.game_copy.game_map.set_cell_occupied_this_round(i, j, False)

    def advance_game(self, commands, player_id, ship_moves={}):
        if self.do_performance_debug:
            # Profiling
            start = timeit.default_timer()

        # Get current current_player
        current_player = self.game_copy.players[player_id]

        # Iterate through all commands
        for command in commands:
            # Split command string
            split_command = command.split(" ")

            # -------------------------BEGIN COMMANDS---------------------------------
            # COMMAND - Generate (spawn ship)
            if split_command[0] == "g":
                if self.generate_new_ships:
                    if current_player.halite_amount >= constants.SHIP_COST:
                        # Get id for newly spawned ship
                        new_id = self.get_next_ship_id()
                        # Spawn ship
                        new_ship = Ship(current_player.id, new_id, current_player.shipyard.position, 0)
                        # Add ship to current_player
                        self.game_copy.players[current_player.id].add_ship(new_ship, new_id)
                        # Reduce current_player halite based on ship cost
                        self.game_copy.players[current_player.id].add_player_halite(-constants.SHIP_COST)
                        # Add ship to cell queue
                        self.game_copy.game_map.add_ship_to_cell_queue(current_player.shipyard.position, self.game_copy.players[current_player.id].get_ship(new_ship.id))
                        # Add cell position to cleaning list
                        self.cells_to_clean.append(current_player.shipyard.position)
                    else:
                        if self.do_warnings:
                            # Log warning if current_player does not have enough money for generating new ship
                            logging.warning("Player "+str(player_id)+" wanted to build ship, but doesn't have enough halite")
            # COMMAND - Move ship
            elif split_command[0] == "m":
                # Get ship id from command
                ship_id = int(split_command[1])
                # Get command for ship
                ship_command = split_command[2]
                # If current_player has this ship
                if self.game_copy.players[player_id].has_ship(ship_id):
                    if self.do_debug:
                        logging.info("All ship moves: " + str(ship_moves))
                    # If we have a command from monte carlo
                    if ship_id in ship_moves:
                        monte_carlo_ship_moves = ship_moves[ship_id]
                        if self.do_debug:
                            logging.info("Ship moves: " + str(monte_carlo_ship_moves))
                        if len(monte_carlo_ship_moves) > 0:
                            if self.do_debug:
                                logging.info("Ship moves[0]: " + str(monte_carlo_ship_moves[0]))
                            # get mcts ship command
                            ship_command = (monte_carlo_ship_moves[0].split(" "))[2]
                            # Remove the command from the list
                            del ship_moves[ship_id][0]
                    # Get ship in question
                    current_ship = self.game_copy.players[player_id].get_ships_dict()[ship_id]
                    # Get ships current cell
                    current_cell = self.game_copy.game_map[current_ship.position]
                    # Get current ship position
                    #ship_position = ship_to_move.position
                    # Get current ship halite amount
                    #ship_halite = ship_to_move.halite_amount

                    # Check if ship on shipyard
                    on_shipyard = False
                    if current_cell.position == current_player.shipyard.position:
                        on_shipyard = True

                    # Has ship failed to move
                    failed_to_move = False
                    # IF NOT STAY STILL
                    if ship_command != "o":
                        # Movement cost for ship
                        move_cost = int(round((1/constants.MOVE_COST_RATIO) * current_cell.halite_amount))

                        # If ship has enough halite to move or is on shipyard
                        if current_ship.halite_amount >= move_cost or on_shipyard:
                            # Remove ship from current cell
                            self.game_copy.game_map.remove_ship_from_cell(current_cell.position)
                            # Subtract move cost from ship if not on shipyard
                            if not on_shipyard:
                                self.game_copy.players[current_player.id].add_ship_halite(current_ship.id, -move_cost)

                            # Change position based on direction
                            if split_command[2] == "n":
                                if self.do_debug:
                                    logging.info("Ship moving north")
                                current_ship.position = self.move(current_ship.position, 0, 1)
                            elif split_command[2] == "s":
                                if self.do_debug:
                                    logging.info("Ship moving south")
                                current_ship.position = self.move(current_ship.position, 0, -1)
                            elif split_command[2] == "e":
                                if self.do_debug:
                                    logging.info("Ship moving east")
                                current_ship.position = self.move(current_ship.position, 1, 0)
                            elif split_command[2] == "w":
                                if self.do_debug:
                                    logging.info("Ship moving west")
                                current_ship.position = self.move(current_ship.position, -1, 0)

                            # Set new ship position
                            self.game_copy.players[current_player.id].change_ship_position(current_ship.id, current_ship.position)

                            # Put ship in queue for next cell
                            self.game_copy.game_map.add_ship_to_cell_queue(current_ship.position, self.game_copy.players[current_player.id].get_ship(current_ship.id))
                            # Add cell position to cleanup map
                            self.cells_to_clean.append(current_ship.position)

                            # Check if ship going to shipyard and deploy Halite
                            if current_ship.position == self.game_copy.players[current_player.id].shipyard.position:
                                self.game_copy.players[current_player.id].add_player_halite(current_ship.halite_amount)
                                self.game_copy.players[current_player.id].set_ship_halite(current_ship.id, 0)

                            # Old collision checking way
                            '''
                            next_cell = self.game_copy.game_map[current_ship.position]
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
                                next_cell.ship = current_ship
                            next_cell.occupied_this_round = True
                            self.game_copy.game_map[current_ship.position].occupied_this_round = True
                            #self.game_copy.game_map[ship_to_move.position] = next_cell
                            '''
                        # If ship does not have enough halite to move
                        else:
                            failed_to_move = True
                            # Log warning for failed to move
                            if self.do_warnings:
                                logging.warning(str(ship_id) +" has "+ str(current_ship.halite_amount))
                                logging.warning("Player " + str(player_id) + " wanted to move ship " + str(ship_id) + ", but doesn't have enough halite")

                    # COMMAND - Stay (mine) or if ship failed to move
                    if ship_command == "o" or failed_to_move:
                        if self.do_debug:
                            logging.info("Ship staying still")
                        # Amount ship will gather
                        gather_amount = int(round((1/constants.EXTRACT_RATIO) * current_cell.halite_amount))
                        # Check if gather amount would go over ship maximum
                        if (current_ship.halite_amount + gather_amount) > hlt.constants.MAX_HALITE:
                            gather_amount = hlt.constants.MAX_HALITE - current_ship.halite_amount
                        # Increase ship halite
                        self.game_copy.players[current_player.id].add_ship_halite(current_ship.id, gather_amount)
                        # Decrease cell halite
                        self.game_copy.game_map.add_cell_halite(current_ship.position, -gather_amount)

                        # Add ship to queue for same cell
                        self.game_copy.game_map.add_ship_to_cell_queue(current_ship.position, self.game_copy.players[
                            current_player.id].get_ship(current_ship.id))
                        # Add cell position to cleanup map
                        self.cells_to_clean.append(current_ship.position)

                        # Remove ship from current cell
                        self.game_copy.game_map.remove_ship_from_cell(current_cell.position)
                        # Add score for ship (Monte carlo reward)
                        self.add_halite_score_to_ship(current_ship.id, gather_amount)

                # If current_player does not have this ship we write a warning to log file
                else:
                    if self.do_warnings:
                        logging.warning(
                            "Player " + str(player_id) + " wanted to move ship "+str(ship_id)+", but doesn't have it")
                        logging.warning(str(self.game_copy.players[player_id].get_ships_dict()))
                #print("move")

            elif split_command[0] == "c":
                # TODO implement consturc dropoff
                print("Make dropoff")

        if self.do_performance_debug:
            # Profiling
            end = timeit.default_timer()
            self.executing_moves_sum = self.executing_moves_sum + (end - start)
            # Profiling
            start = timeit.default_timer()

        # Clean up map and do collisions
        self.clean_map()

        if self.do_performance_debug:
            # Profiling
            end = timeit.default_timer()
            self.map_cleanup_sum = self.map_cleanup_sum + (end - start)

    # Run this with the parameter containing the ship moves from monte carlo where the key is an int (ship id) and the value is a LIST of commands
    def run_simulation(self, default_policy_bot, ship_moves={}):
        self.reset_simulator()
        while self.game_copy.turn_number < self.search_depth:
            if self.do_debug:
                # Print map for DEBUGGING
                self.print_map()
            # Print turn number to log file
            if self.do_debug:
                logging.info("+++++++++ TURN {:03} +++++++++ :SIM".format(self.game_copy.turn_number))

            if self.do_performance_debug:
                # Profiling - default policy time
                start = timeit.default_timer()
            # Run default policy
            returns = default_policy_bot.run(self.game_copy)

            if self.do_performance_debug:
                # Profiling end and add to sum
                end = timeit.default_timer()
                self.bot_time_sum = self.bot_time_sum + (end - start)

            # Advance game based on returns of the bot
            self.advance_game(returns[0], returns[1], ship_moves)

            # Increment turn by one
            self.game_copy.turn_number += 1

        if self.do_performance_debug:
            bot.profiling.print_ms_message("Deep copy took: ", self.deep_copy_time_sum)
            bot.profiling.print_ms_message("Simulation took: ", self.executing_moves_sum + self.map_cleanup_sum)
            bot.profiling.print_ms_message("Advance moves: ", self.executing_moves_sum)
            bot.profiling.print_ms_message("Clean Up Map: ", self.map_cleanup_sum)
            bot.profiling.print_ms_message("Bot took: ", self.bot_time_sum)

        if self.do_debug:
            logging.info("Turn number: " + str(self.game_copy.turn_number))
        return self.ship_scores
