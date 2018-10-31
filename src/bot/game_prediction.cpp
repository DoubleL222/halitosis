#include "game_prediction.hpp"

GameClone::GameClone(const hlt::Game & game)
{
	this->map = hlt::GameMap(*game.game_map);
}

void GameClone::advance_game(Plan & plan, hlt::Ship & ship, Frame & frame)
{
	for (int i = plan.execution_step; i < plan.path.size(); i++)
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
			hlt::MapCell * next_cell = map.at(next_pos);
			next_cell->ship = ship_cell->ship;
			ship_cell->ship = nullptr;
		}
	}
}

GamePrediction::GamePrediction(const hlt::Game & game, std::unordered_map<hlt::EntityId, Plan> plans, int prediction_steps)
{
	prediction_map = std::make_unique<int[]>(game.game_map->height * game.game_map->width * prediction_steps);
}
