#pragma once

#include "hlt/command.hpp"
#include "hlt/game.hpp"
#include "bot/typedefs.hpp"

#include <vector>

class Bot {
    virtual void init(hlt::Game& game) = 0;
    virtual std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time) = 0;
};
