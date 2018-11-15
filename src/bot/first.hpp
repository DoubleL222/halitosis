#pragma once

#include "bot/bot.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"

#include <random>
#include <vector>

struct Plan {
    SearchPath path;
    unsigned int execution_step;

    Plan();
    Plan(SearchPath path);

    bool is_finished() const;

    hlt::Direction next_move() const;
    void advance();
};

class FirstBot : public Bot {
    std::mt19937 rng;
    std::unordered_map<hlt::EntityId, Plan> plans;
    bool should_build_ship;

public:
    FirstBot(unsigned int seed);

    void init(hlt::Game& game);

    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);

private:
	hlt::Game advance_game(hlt::Game& game, std::vector<hlt::Command> moves);
};
