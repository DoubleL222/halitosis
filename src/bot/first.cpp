#include "bot/first.hpp"
#include "bot/frame.hpp"
#include "bot/game_clone.hpp"

Plan::Plan()
  : path(SearchPath()),
    execution_step(0)
{
}

Plan::Plan(SearchPath path)
  : path(path),
    execution_step(0)
{
}

bool Plan::is_finished() {
    return execution_step >= path.size();
}

hlt::Direction Plan::next_move() const {
    return path[execution_step].direction;
}

void Plan::advance() {
    execution_step++;
}

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
	//Make game clone
	GameClone game_clone(frame, true, MAX_SEARCH_DEPTH);

    auto player = frame.get_game().me;

    std::vector<hlt::Command> commands;

	//Updating clone map
	for (auto& pair : player->ships) {
		auto& ship = pair.second;
		game_clone.advance_game(plans[ship->id], *ship);
	}

    if (
        player->halite >= 1000
        && !game.game_map->at(player->shipyard)->is_occupied()
        && !game_clone.is_cell_occupied(player->shipyard->position, 1)
        && should_build_ship
    ) {
        commands.push_back(player->shipyard->spawn());
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
            auto max_depth = std::min(MAX_SEARCH_DEPTH, turns_left);

            auto now = ms_clock::now();
            //Make path on the map clone
            auto path = frame.get_optimal_path(
                game_clone.get_map(),
                *ship,
                player->shipyard->position,
                now+time_per_plan,
                max_depth);

            plans[id] = Plan(path.max_per_turn);
            if (path.max_total.size() > 0) {
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
        if (plans[id].is_finished()) {
            moves[id] = hlt::Direction::STILL;
        } else {
            moves[id] = plans[id].next_move();
        }
    }

    //Collision avoidance,
    auto new_moves = frame.avoid_collisions(moves, turns_left < 15);

    // Find moves for each ship, avoiding collisions.
    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        if (moves[id] == new_moves[id]) {
            plans[id].advance();
        }
        commands.push_back(ship->move(new_moves[id]));
	}
    return commands;
}
