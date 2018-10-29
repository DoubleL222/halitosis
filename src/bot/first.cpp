#include "bot/first.hpp"
#include "bot/frame.hpp"

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

hlt::Direction Plan::next_move() {
    auto dir = path[execution_step];
    execution_step++;
    return dir;
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
	//Advance game state from remaining plans
	//game_clone.advance_game()

    auto player = frame.get_game().me;

    std::vector<hlt::Command> commands;
    if (game.turn_number == 1) {
        commands.push_back(player->shipyard->spawn());
    }
    for (auto& pair: player->ships) {
        auto& ship = pair.second;
        if (plans[ship->id].is_finished()) {
            auto path = frame.get_optimal_path(*ship, player->shipyard->position);
            plans[ship->id] = Plan(path);
        }
        commands.push_back(ship->move(plans[ship->id].next_move()));



        //if (ship->position == player->shipyard->position) {
        //}
    }
    return commands;
}


/*
        std::unique_ptr<hlt::GameMap>& game_map = game.game_map;

        std::vector<hlt::Command> command_queue;

        for (const auto& ship_iterator : me->ships) {
            shared_ptr<Ship> ship = ship_iterator.second;
            if (game_map->at(ship)->halite < constants::MAX_HALITE / 10 || ship->is_full()) {
                Direction random_direction = ALL_CARDINALS[rng() % 4];
            } else {
                command_queue.push_back(ship->stay_still());
            }
        }

        if (
            game.turn_number <= 200 &&
            me->halite >= constants::SHIP_COST &&
            !game_map->at(me->shipyard)->is_occupied())
        {
            command_queue.push_back(me->shipyard->spawn());
        }
*/

GameClone::GameClone(const hlt::Game & game)
{
	this->map = hlt::GameMap(*game.game_map);
}

void GameClone::advance_game(Plan & plan, hlt::Ship & ship, Frame & frame)
{
	for (int i = 0; i<plan.path.size; i++)
	{
		hlt::MapCell * ship_cell = map.at(ship);

		if (plan.path[i] == hlt::Direction::STILL)
		{
			//Mine The Cell
			ship_cell->halite = ship_cell->halite * 0.75f;
		}
		else 
		{
			hlt::Position next_pos = frame.move(ship.position, plan.path[i]);
			ship_cell->is_empty = true;
			hlt::MapCell * next_cell = map.at(next_pos);
			next_cell->ship = ship_cell->ship;
			ship_cell->ship = nullptr;
		}
	}
}
