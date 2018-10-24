#pragma once

#include "hlt/game.hpp"

// A wrapper around the game with helper methods for the current frame of the game.
class Frame {
    const hlt::Game& game;

public:
    Frame(const hlt::Game& game);

    const hlt::Game& get_game() const;
};
