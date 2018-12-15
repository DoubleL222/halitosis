#pragma once

#include "bot/bot.hpp"

class MctsBot : public Bot {
    unsigned int seed;

public:
    MctsBot(unsigned int seed);

    void init(hlt::Game& game);
    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);
};
