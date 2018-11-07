﻿#pragma once

#include "hlt/direction.hpp"
#include "hlt/game.hpp"
#include "hlt/position.hpp"
#include "hlt/map_cell.hpp"

using Path = std::vector<hlt::Direction>;

// A wrapper around the game with helper methods for the current frame of the game.
class Frame {
    const hlt::Game& game;

public:
    Frame(const hlt::Game& game);

	enum class DistanceMeasure : char {
		EUCLIDEAN = 'e',
		MANHATTAN = 'm'
	};

    const hlt::Game& get_game() const;

    hlt::Position move(hlt::Position pos, int direction_x, int direction_y);
    hlt::Position move(hlt::Position pos, hlt::Direction direction);

    Path get_optimal_path(hlt::Ship& ship, hlt::Position end);
    Path get_optimal_path(hlt::GameMap& map, hlt::Ship& ship, hlt::Position end);

	//Direct path
	Path get_direct_path(hlt::GameMap& map, hlt::Ship& ship, hlt::Position end);

    // Update the moves map to avoid collisions between own ships.
    std::unordered_map<hlt::EntityId, hlt::Direction> avoid_collisions(
        std::unordered_map<hlt::EntityId, hlt::Direction>& moves
    );

	std::vector<hlt::MapCell*> get_cells_within_radius(const hlt::Position& center, const int radius, const DistanceMeasure distanceMeasure = DistanceMeasure::MANHATTAN);

	int get_total_halite_in_cells_within_radius(const hlt::Position& center, const int radius, const DistanceMeasure distanceMeasure = DistanceMeasure::MANHATTAN);

	//for finding optimal ship count
	int get_optimal_number_of_ships(float factor_halite_left, float factor_turns_left);

	int get_helite_left_on_map(hlt::GameMap & map);

	int get_halite_left();

	int get_turns_left();

    // Retrieve the number of cells on the board
    unsigned int get_board_size() const;

private:
    // Converts a position to an index in the grid when stored as a row-order matrix.
    int get_index(hlt::Position pos) const;
};
