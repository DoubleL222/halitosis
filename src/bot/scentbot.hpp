#pragma once

#include "bot/bot.hpp"
#include "bot/plan.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"

#include <random>
#include <vector>

class ScentBot : public Bot {
    std::mt19937 rng;
    std::unordered_map<hlt::EntityId, Plan> plans;

public:
    ScentBot(unsigned int seed);

    void init(hlt::Game& game);

    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);
};
