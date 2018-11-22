#include "bot/first.hpp"
#include "bot/frame.hpp"
#include "bot/game_clone.hpp"


hlt::Game FirstBot::advance_game(hlt::Game& game, std::vector<hlt::Command> moves)
{
	return hlt::Game();
}

FirstBot::FirstBot(unsigned int seed)
  : rng(seed),
    should_build_ship(true)
{
}

void FirstBot::init(hlt::Game& game) {
    // Last thing to call
    game.ready("FirstBot");
}

// Max depth used by get_optimal_path
const unsigned int MAX_SEARCH_DEPTH = 200;
// Assume that a new ship will collect halite at this rate compared to the most recent ship for
// the rest of the game.
const float SHIP_BUILD_FACTOR = 0.5;

std::vector<hlt::Command> FirstBot::run(const hlt::Game& game, time_point end_time) {
    Frame frame(game);
    auto player = game.me;

	//Make game clone
	GameClone game_clone(frame);
	for (auto& pair : player->ships) {
		auto& ship = pair.second;
		game_clone.advance_game(plans[ship->id], *ship);
	}

    std::unordered_map<hlt::EntityId, hlt::Direction> moves;
    int num_finished_plans = 0;
    for (auto& pair : player->ships) {
        if (plans[pair.first].is_finished()) { num_finished_plans++; }
    }
    auto time_budget = end_time-ms_clock::now();
        //std::chrono::duration_cast<std::chrono::milliseconds>();
    auto time_per_plan = time_budget;
    if (num_finished_plans != 0) { time_per_plan /= num_finished_plans; }

    unsigned int turns_left = hlt::constants::MAX_TURNS-frame.get_game().turn_number;
    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        // Create plans if necessary.
        if (plans[ship->id].is_finished()) {
            auto max_depth = std::min(MAX_SEARCH_DEPTH, turns_left-4);

            auto now = ms_clock::now();
            unsigned int defensive_turns = (game.players.size() == 4 ? 150 : 0);
            //Make path on the map clone
            auto path = game_clone.get_optimal_path(
                *ship,
                player->shipyard->position,
                now+time_per_plan,
                max_depth,
                defensive_turns);

            plans[id] = Plan(path.max_per_turn);
            if (path.max_total.size() > 0) { // Prevents a crash when game is ending
                auto worth = path.max_per_turn[path.max_per_turn.size()-1].halite;
                auto per_turn = ((float)worth)/path.max_per_turn.size();
                auto expected_total = turns_left*per_turn;
                if (SHIP_BUILD_FACTOR*expected_total < hlt::constants::SHIP_COST) {
                    should_build_ship = false;
                }
            }

            //Update clone map with current plan
            game_clone.advance_game(plans[id], *ship);
        }
        moves[id] = plans[id].next_move();
    }

    frame.ensure_moves_possible(moves);
    if (game.players.size() == 4) {
        frame.avoid_enemy_collisions(moves);
    }

    // Whether to attempt to build a ship.
    bool spawn_desired = false;
    if (player->halite >= 1000 && should_build_ship) {
        spawn_desired = true;
    }

    //Collision avoidance,
    auto collision_res = frame.avoid_collisions(moves, turns_left < 15, spawn_desired);

    std::vector<hlt::Command> commands;
    // Find moves for each ship, avoiding collisions.
    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        if (plans[id].next_move() == collision_res.safe_moves[id]) {
            plans[id].advance();
        }
        commands.push_back(ship->move(collision_res.safe_moves[id]));
	}
    if (collision_res.is_spawn_possible) {
        commands.push_back(player->shipyard->spawn());
    }
    return commands;
}
