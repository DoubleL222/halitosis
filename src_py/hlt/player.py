from .entity import Shipyard, Ship, Dropoff
from .positionals import Position
from .common import read_input
import logging

class Player:
    """
    Player object containing all items/metadata pertinent to the player.
    """
    def __init__(self, player_id, shipyard, halite=0):
        self.id = player_id
        self.shipyard = shipyard
        self.halite_amount = halite
        self._ships = {}
        self._dropoffs = {}

    def get_ship(self, ship_id):
        """
        Returns a singular ship mapped by the ship id
        :param ship_id: The ship id of the ship you wish to return
        :return: the ship object.
        """
        return self._ships[ship_id]

    def get_ships_dict(self):
        """
        :return: all ship dictionary
        """
        return self._ships

    def get_ships(self):
        """
        :return: Returns all ship objects in a list
        """
        return list(self._ships.values())

    def get_dropoff(self, dropoff_id):
        """
        Returns a singular dropoff mapped by its id
        :param dropoff_id: The dropoff id to return
        :return: The dropoff object
        """
        return self._dropoffs[dropoff_id]

    def get_dropoffs(self):
        """
        :return: Returns all dropoff objects in a list
        """
        return list(self._dropoffs.values())

    def add_ship(self, ship, ship_id):
        """
        :param ship: ship object to add
        :param ship_id: ship id to add
        :return: nothing
        """
        self._ships[ship_id] = ship

    def remove_ship(self, ship_id):
        logging.warning("shipID " + str(ship_id) + " ;" + str(self._ships))
        if ship_id in self._ships:
            del self._ships[ship_id]

    def add_ship_halite(self, ship_id, halite_to_add):
        self._ships[ship_id].halite_amount += halite_to_add

    def set_ship_halite(self, ship_id, halite_to_set):
        self._ships[ship_id].halite_amount = halite_to_set

    def add_player_halite(self, halite_to_add):
        self.halite_amount += halite_to_add

    def change_ship_position(self, ship_id, new_position):
        self._ships[ship_id].position = new_position

    def has_ship(self, ship_id):
        """
        Check whether the player has a ship with a given ID.

        Useful if you track ships via IDs elsewhere and want to make
        sure the ship still exists.

        :param ship_id: The ID to check.
        :return: True if and only if the ship exists.
        """
        return ship_id in self._ships


    @staticmethod
    def _generate():
        """
        Creates a player object from the input given by the game engine
        :return: The player object
        """
        player, shipyard_x, shipyard_y = map(int, read_input().split())
        return Player(player, Shipyard(player, -1, Position(shipyard_x, shipyard_y)))

    def _update(self, num_ships, num_dropoffs, halite):
        """
        Updates this player object considering the input from the game engine for the current specific turn.
        :param num_ships: The number of ships this player has this turn
        :param num_dropoffs: The number of dropoffs this player has this turn
        :param halite: How much halite the player has in total
        :return: nothing.
        """
        self.halite_amount = halite
        self._ships = {id: ship for (id, ship) in [Ship._generate(self.id) for _ in range(num_ships)]}
        self._dropoffs = {id: dropoff for (id, dropoff) in [Dropoff._generate(self.id) for _ in range(num_dropoffs)]}
