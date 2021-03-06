﻿#pragma once

#include "hlt/direction.hpp"
#include "hlt/game.hpp"
#include "hlt/position.hpp"
#include "hlt/map_cell.hpp"
#include "bot/typedefs.hpp"

// Must match all_directions
enum Move {
    STILL_INDEX,
    NORTH_INDEX,
    SOUTH_INDEX,
    EAST_INDEX,
    WEST_INDEX
};
static const std::array<hlt::Direction, 5> ALL_DIRECTIONS = {
    hlt::Direction::STILL,
    hlt::Direction::NORTH,
    hlt::Direction::SOUTH,
    hlt::Direction::EAST,
    hlt::Direction::WEST
};

struct PathSegment {
    hlt::Direction direction;
    hlt::Halite halite;
    int mining_idx;

    PathSegment();
    PathSegment(hlt::Direction direction, hlt::Halite halite, int mining_idx);
};

using Path = std::vector<hlt::Direction>;
using SearchPath = std::vector<PathSegment>;

struct OptimalPath {
    unsigned int search_depth;
    hlt::Halite final_halite;
    SearchPath path;
};

struct SpawnRes {
    bool is_spawn_possible;
    std::unordered_map<hlt::EntityId, hlt::Direction> safe_moves;
};

// A wrapper around the game with helper methods for the current frame of the game.
class Frame {
    const hlt::Game& game;
    std::unordered_map<hlt::EntityId, hlt::Direction> last_moves;

public:
    Frame(const hlt::Game& game);
    Frame(const hlt::Game& game, std::unordered_map<hlt::EntityId, hlt::Position>& previous_positions);

	enum class DistanceMeasure : char {
		EUCLIDEAN = 'e',
		MANHATTAN = 'm'
	};

    const hlt::Game& get_game() const;

    hlt::Position move(hlt::Position pos, int direction_x, int direction_y) const;
    hlt::Position move(hlt::Position pos, hlt::Direction direction) const;

    hlt::PlayerId get_closest_shipyard(hlt::Position pos);

    void ensure_moves_possible(std::unordered_map<hlt::EntityId, hlt::Direction>& moves);

    void avoid_enemy_collisions(std::unordered_map<hlt::EntityId, hlt::Direction>& moves);

    // Update the moves map to avoid collisions between own ships.
    SpawnRes avoid_collisions(
        std::unordered_map<hlt::EntityId, hlt::Direction>& moves,
        bool ignore_collisions_at_dropoff,
        // Whether a the controller wants to spawn a ship.
        bool spawn_desired
    );

	std::vector<hlt::MapCell*> get_cells_within_radius(const hlt::Position& center, const int radius, const DistanceMeasure distanceMeasure = DistanceMeasure::MANHATTAN);

	int get_total_halite_in_cells_within_radius(const hlt::Position& center, const int radius, const DistanceMeasure distanceMeasure = DistanceMeasure::MANHATTAN);

    bool ship_at(hlt::Position pos) const;

    // Retrieve the number of cells on the board
    unsigned int get_board_size() const;

    // Converts a position to an index in the grid when stored as a row-order matrix.
    int get_index(hlt::Position pos) const;

    int get_depth_index(int depth, hlt::Position pos) const;
private:
    hlt::Position indexToPosition(int idx);
};
