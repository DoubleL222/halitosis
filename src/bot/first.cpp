#include "bot/first.hpp"
#include "bot/frame.hpp"
#include "bot/game_prediction.hpp"

Plan::Plan()
  : path(Path()),
    execution_step(0)
{
}

Plan::Plan(Path path)
  : path(path),
    execution_step(0)
{
}

bool Plan::is_finished() {
    return execution_step >= path.size();
}

hlt::Direction Plan::next_move() const {
    return path[execution_step];
}

void Plan::advance() {
    execution_step++;
}

hlt::Game FirstBot::advance_game(hlt::Game& game, std::vector<hlt::Command> moves)
{
	
	return hlt::Game();
}

FirstBot::FirstBot(unsigned int seed)
    : rng(seed)
{
}

void FirstBot::init(hlt::Game& game) {
    // Last thing to call
    game.ready("FirstBot");
}

std::vector<hlt::Command> FirstBot::run(const hlt::Game& game) {
    Frame frame(game);
	//Make game clone
	GameClone game_clone = GameClone(game);

	GamePrediction game_prediction(frame, plans, 50);

	hlt::log::log(game_prediction.print_prediction());

    auto player = frame.get_game().me;

    std::vector<hlt::Command> commands;
    if (game.turn_number / hlt::constants::MAX_TURNS < 0.6 && player->halite >= 1000 && !game.game_map->at(player->shipyard)->is_occupied()) {
        commands.push_back(player->shipyard->spawn());
    }

	//Updating clone map
	for (auto& pair : player->ships) {
		auto id = pair.first;
		auto& ship = pair.second;
		game_clone.advance_game(plans[ship->id], *ship, frame);
	}

    std::unordered_map<hlt::EntityId, hlt::Direction> moves;
    for (auto& pair: player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        // Create plans if necessary.
        if (plans[ship->id].is_finished()) {
            //auto path = frame.get_optimal_path(*ship, player->shipyard->position);
            
			//Make path on the map clone
			auto path = frame.get_optimal_path(game_clone.map, *ship, player->shipyard->position);

			plans[id] = Plan(path);

			//Update clone map with current plan
			game_clone.advance_game(plans[id], *ship, frame);
        }
        moves[id] = plans[id].next_move();
    }
    auto new_moves = frame.avoid_collisions(moves);

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
