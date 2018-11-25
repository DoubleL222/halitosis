#!/usr/bin/env python3
# Python 3.6

# Import the Halite SDK, which will let you interact with the game.
import hlt

# This library contains constant values.
from hlt import constants

# This library contains direction metadata to better interface with the game.
from hlt.positionals import Direction

# This library allows you to generate random numbers.
import random

# Logging allows you to save messages for yourself. This is required because the regular STDOUT
#   (print statements) are reserved for the engine-bot communication.
import logging

from mcts import mcts
import time

""" <<<Game Begin>>> """

# This game object contains the initial game state.
game = hlt.Game()
# At this point "game" variable is populated with initial map data.
# This is a good place to do computationally expensive start-up pre-processing.
# As soon as you call "ready" function below, the 2 second per turn timer will start.
game.ready("MyPythonBot")

# Now that your bot is initialized, save a message to yourself in the log file with some important information.
#   Here, you log here your id, which you can always fetch from the game object by using my_id.
logging.info("Successfully created bot! My Player ID is {}.".format(game.my_id))

""" <<<Game Loop>>> """

while True:
    # This loop handles each turn of the game. The game object changes every turn, and you refresh that state by
    #   running update_frame().
    game.update_frame()
    # You extract player metadata and the updated map metadata here for convenience.
    me = game.me
    game_map = game.game_map

    # A command queue holds all the commands you will run this turn. You build this list up and submit it at the
    #   end of the turn.
    command_queue = []

    # for ship in me.get_ships():
    #     # For each of your ships, move randomly if the ship is on a low halite location or the ship is full.
    #     #   Else, collect halite.
    #     if game_map[ship.position].halite_amount < constants.MAX_HALITE / 10 or ship.is_full:
    #         command_queue.append(
    #             ship.move(
    #                 random.choice([ Direction.North, Direction.South, Direction.East, Direction.West ])))
    #     else:
    #         command_queue.append(ship.stay_still())

    # Reset best action lists
    mcts.Mcts.ship_best_action_lists = {}

    # Create MCTS runners for each ship
    mcts_runners = []
    for ship in me.get_ships():
        mcts_runners.append(mcts.Mcts(ship_id=ship.id, game_state=game, current_turn=game.turn_number,
                                      game_max_turns=constants.MAX_TURNS))

    # Timestamp = current time + time limit (all values are in seconds and are floats)
    max_iterations = 100
    num_iterations_done = 0
    time_limit = time.time() + 1.0
    last_iteration_time = 0.0
    while True:
        iteration_start_time = time.time()
        # Use last iteration's time spent, to approximate the amount of time we would spend on the next iteration,
        # and if we would exceed the allotted time_limit, break before doing anything.
        if iteration_start_time + last_iteration_time >= time_limit:
            break

        for mcts_runner in mcts_runners:
            mcts_runner.do_one_uct_update()

        last_iteration_time = time.time() - iteration_start_time
        num_iterations_done += 1
        if num_iterations_done >= max_iterations:
            break

    logging.info("MCTS ran for " + str(num_iterations_done) + " iterations.")

    action_list_dict = mcts.Mcts.ship_best_action_lists
    for ship in me.get_ships():
        # Debugging stuff
        action_list = action_list_dict[ship.id]
        actions = ""
        for action in action_list:
            actions += action + ", "
        logging.info("Ship " + str(ship.id) + " best action list after update: " + actions)

        action = None
        if ship.id in action_list_dict:
            action = action_list_dict[ship.id][0]
            logging.info("Ship is doing MCTS action: " + action)
        else:
            action = random.choice([Direction.North, Direction.South, Direction.East, Direction.West])
            logging.info("Ship is doing random action: " + action)

        # Append the chosen action to the command queue.
        command_queue.append(ship.move(action))

    # If the game is in the first 200 turns and you have enough halite, spawn a ship.
    # Don't spawn a ship if you currently have a ship at port, though - the ships will collide.
    if game.turn_number <= 200 and me.halite_amount >= constants.SHIP_COST and not game_map[me.shipyard].is_occupied:
        command_queue.append(me.shipyard.spawn())

    # Send your moves back to the game environment, ending this turn.
    game.end_turn(command_queue)

