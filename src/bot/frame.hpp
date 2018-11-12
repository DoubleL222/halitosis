#pragma once

#include "hlt/direction.hpp"
#include "hlt/game.hpp"
#include "hlt/position.hpp"
#include "hlt/map_cell.hpp"
#include "bot/typedefs.hpp"

struct PathSegment {
    hlt::Direction direction;
    hlt::Halite halite;

    PathSegment();
    PathSegment(hlt::Direction direction, hlt::Halite halite);
};

using Path = std::vector<hlt::Direction>;
using SearchPath = std::vector<PathSegment>;

struct OptimalPath {
    SearchPath max_per_turn;
    SearchPath max_total;
};

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

    OptimalPath get_optimal_path(
        hlt::GameMap& map,
        hlt::Ship& ship,
        hlt::Position end,
        time_point end_time,
        unsigned int max_depth);

	//Direct path
	Path get_direct_path(hlt::GameMap& map, hlt::Ship& ship, hlt::Position end);

    void ensure_moves_possible(std::unordered_map<hlt::EntityId, hlt::Direction>& moves);

    // Update the moves map to avoid collisions between own ships.
    std::unordered_map<hlt::EntityId, hlt::Direction> avoid_collisions(
        std::unordered_map<hlt::EntityId, hlt::Direction>& moves,
        bool ignore_collisions_at_dropoff
    );

	std::vector<hlt::MapCell*> get_cells_within_radius(const hlt::Position& center, const int radius, const DistanceMeasure distanceMeasure = DistanceMeasure::MANHATTAN);

	int get_total_halite_in_cells_within_radius(const hlt::Position& center, const int radius, const DistanceMeasure distanceMeasure = DistanceMeasure::MANHATTAN);

    // Retrieve the number of cells on the board
    unsigned int get_board_size() const;
private:
    // Converts a position to an index in the grid when stored as a row-order matrix.
    int get_index(hlt::Position pos) const;

    hlt::Position indexToPosition(int idx);
};
