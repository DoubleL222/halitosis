#pragma once

#include "bot/bot.hpp"
#include "bot/gravity_grid.hpp"

class MctsBot : public Bot {
    // Rng
    std::mt19937 generator;
    // Grid shared by all players for mining purposes
    GravityGrid mining_grid;
    // Grids for each player used to return to dropoffs
    std::vector<GravityGrid> return_grids;

public:
    MctsBot(unsigned int seed);

    void init(hlt::Game& game);
    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);
};
