# Import the Halite SDK, which will let you interact with the game.
import hlt
# This library contains constant values.
from hlt import constants
# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

# This library allows you to generate random numbers.
import random

import logging

class RandomBot:
    def __init__(self, game, id):
        self.game = game
        self.id = id

    def run(self, game):
        #print("RUN")
        logging.info("RUN")
        command_queue = []
        me = game.players[self.id]
        #print("Ships: " + str(len(me.get_ships())))
        game_map = game.game_map

        for ship in me.get_ships():
            #print("IN SHIPS FOR LOOP")
            logging.info("IN SHIPS FOR LOOP")
            # For each of your ships, move randomly if the ship is on a low halite location or the ship is full.
            #   Else, collect halite.
            if game_map[ship.position].halite_amount < constants.MAX_HALITE / 10 or ship.is_full:
                #print("SHOULD MAKE MOVE")
                command_queue.append(
                    ship.move(
                        random.choice([Direction.North, Direction.South, Direction.East, Direction.West])))
            else:
                command_queue.append(ship.stay_still())

        # If the game is in the first 200 turns and you have enough halite, spawn a ship.
        # Don't spawn a ship if you currently have a ship at port, though - the ships will collide.
        if game.turn_number <= 200 and me.halite_amount >= constants.SHIP_COST and not game_map[
            me.shipyard].is_occupied:
            command_queue.append(me.shipyard.spawn())

        return [command_queue, self.id]
