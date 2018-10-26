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
};
