#pragma once

#include "bot/bot.hpp"
#include "bot/plan.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"

#include <random>
#include <vector>

struct FirstBotArgs {
    // Name of the bot
    std::string name;

    // Forcibly terminate the bot after this number of moves.
    int max_turns;
    // Search depth for get_optimal_path
    int max_search_depth;
    // Assumption that new ships will collect this ratio of what a current ship will collect until
    // the end of the game.
    float ship_build_factor;
    // Whether to include a simulation of enemy movements into the gameclone.
    bool simulate_enemy_enabled;
    // Whether to recalculate paths
    bool recalculate_paths_enabled;
    // Whether to try to avoid collisions with the enemy.
    bool avoid_enemy_collisions_enabled;

    FirstBotArgs();
};

class FirstBot : public Bot {
    FirstBotArgs args;
    std::unordered_map<hlt::EntityId, Plan> plans;
    bool should_build_ship;
    // The last position of each ship, both own and opponents
    std::unordered_map<hlt::EntityId, hlt::Position> previous_positions;
    // How many turns it has been since each of our own ships have been to a dropoff.
    std::unordered_map<hlt::EntityId, unsigned int> current_turns_underway;

public:
    FirstBot(FirstBotArgs args);

    void init(hlt::Game& game);

    std::vector<hlt::Command> run(const hlt::Game& game, time_point end_time);

private:
	hlt::Game advance_game(hlt::Game& game, std::vector<hlt::Command> moves);
    void update_previous_positions(const hlt::Game& game);
};
