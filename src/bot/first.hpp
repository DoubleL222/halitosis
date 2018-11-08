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

    bool is_finished();

    hlt::Direction next_move() const;
    void advance();
};

class FirstBot : public Bot {
    std::mt19937 rng;

    std::unordered_map<hlt::EntityId, Plan> plans;

	hlt::Game advance_game(hlt::Game& game, std::vector<hlt::Command> moves);

	enum bot_state {building_and_gathering, hoarding_and_gathering, returning};

	bot_state current_bot_state;

	//Factors for optimal ship number
	float factor_halite_left;
	float factor_turns_left;

public:
    FirstBot(unsigned int seed);

    void init(hlt::Game& game);

    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);
};
