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

    // The number of turns since the ship last visited a dropoff.
    std::unordered_map<hlt::EntityId, int> turns_underway;
    // Average scores of the previous round's simulations.
    // Used to evaluate this rounds scores.
    std::unordered_map<hlt::EntityId, float> last_average_scores;

public:
    MctsBot(unsigned int seed);

    void init(hlt::Game& game);
    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);

private:
    void maintain(const hlt::Game& game);
};
