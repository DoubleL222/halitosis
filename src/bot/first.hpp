#pragma once

#include "bot/bot.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"

#include <random>
#include <vector>

class FirstBot : public Bot {
    std::mt19937 rng;

public:
    FirstBot(unsigned int seed);

    void init(hlt::Game& game);

    std::vector<hlt::Command> run(const hlt::Game& game);
};
