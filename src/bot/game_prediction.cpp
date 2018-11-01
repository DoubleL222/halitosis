#include "game_prediction.hpp"

GameClone::GameClone(const hlt::Game & game)
{
	this->map = hlt::GameMap(*game.game_map);
}

void GameClone::advance_game(Plan & plan, hlt::Ship & ship, Frame & frame)
{
	hlt::MapCell * ship_cell;
	hlt::MapCell * next_cell;
	for (int i = plan.execution_step; i < plan.path.size(); i++)
	{
		ship_cell = map.at(ship);

		if (plan.path[i] == hlt::Direction::STILL)
		{
			//Mine The Cell
			ship_cell->halite = ship_cell->halite * 0.75f;
		}
		else
		{
			hlt::Position next_pos = frame.move(ship.position, plan.path[i]);
			next_cell = map.at(next_pos);
			next_cell->ship = ship_cell->ship;
			ship_cell->ship = nullptr;
		}
	}

	delete next_cell;
	delete ship_cell;
}

int GamePrediction::get_index_from_cell(hlt::Position map_position, hlt::GameMap & map, int prediction_step)
{
	int index = prediction_step * (map.height * map.width) + (map_position.y * map.width) + map_position.x;
	return index;
}

GamePrediction::GamePrediction(const hlt::Game & game, std::unordered_map<hlt::EntityId, Plan> plans, int prediction_steps)
{
	prediction_map = std::make_unique<int[]>(game.game_map->height * game.game_map->width * prediction_steps);
	
	//Make current state
	for (auto& pair : game.me->ships) 
	{
		auto id = pair.first;
		auto& ship = pair.second;
		prediction_map[get_index_from_cell(ship->position, *game.game_map, 0)] = 1;
	}

	//Make clone of current game state
	GameClone game_clone = GameClone(game);

	//Make future state
	for (auto& pair : game.me->ships) {
		auto id = pair.first;
		auto& ship = pair.second;
		game_clone.advance_game(plans[ship->id], *ship, frame);
	}
}
