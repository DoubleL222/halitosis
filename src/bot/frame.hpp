#pragma once

#include "hlt/direction.hpp"
#include "hlt/game.hpp"
#include "hlt/position.hpp"

using Path = std::vector<hlt::Direction>;

// A wrapper around the game with helper methods for the current frame of the game.
class Frame {
    const hlt::Game& game;

public:
    Frame(const hlt::Game& game);

    const hlt::Game& get_game() const;

    hlt::Position move(hlt::Position pos, int direction_x, int direction_y);
    hlt::Position move(hlt::Position pos, hlt::Direction direction);

    Path get_optimal_path(hlt::Ship& ship, hlt::Position end);
    Path get_optimal_path(hlt::GameMap& map, hlt::Ship& ship, hlt::Position end);

    // Update the moves map to avoid collisions between own ships.
    std::unordered_map<hlt::EntityId, hlt::Direction> avoid_collisions(
        std::unordered_map<hlt::EntityId, hlt::Direction>& moves
    );

private:
    // Converts a position to an index in the grid when stored as a row-order matrix.
    int get_index(hlt::Position pos) const;

    // Attempt to make a ship move a specific way. As long as this leads to a collision, a ship
    // will be selected to stand still instead.
    void avoid_collisions_rec(
        std::unordered_map<hlt::EntityId, hlt::Direction>& current_moves,
        std::unordered_map<hlt::Position, hlt::EntityId>& ship_going_to_position,
        hlt::EntityId ship,
        hlt::Position ship_position,
        hlt::Direction desired_move
    );
};
