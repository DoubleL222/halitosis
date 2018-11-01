#include "game_prediction.hpp"

GameClone::GameClone(const hlt::Game & game)
{
	this->map = hlt::GameMap(*game.game_map);
}

void GameClone::advance_game(Plan & plan, hlt::Ship & ship, Frame & frame)
{
	for (int i = plan.execution_step; i < plan.path.size(); i++)
	{
		//ship_cell = map.at(ship);

		//if (plan.path[i] == hlt::Direction::STILL)
		//{
		//	//Mine The Cell
		//	ship_cell->halite = ship_cell->halite * 0.75f;
		//}
		//else
		//{
		//	hlt::Position next_pos = frame.move(ship.position, plan.path[i]);
		//	next_cell = map.at(next_pos);
		//	next_cell->ship = ship_cell->ship;
		//	ship_cell->ship = nullptr;
		//}
		advance_game_by_step(plan.path[i], ship, frame);
	}
}

void GameClone::advance_game_by_step(hlt::Direction dir, hlt::Ship & ship, Frame & frame)
{
	hlt::MapCell * ship_cell;
	hlt::MapCell * next_cell;

	ship_cell = map.at(ship);

	if (dir == hlt::Direction::STILL)
	{
		//Mine The Cell
		ship_cell->halite = ship_cell->halite * 0.75f;
	}
	else
	{
		//move
		hlt::Position next_pos = frame.move(ship.position, dir);
		next_cell = map.at(next_pos);
		next_cell->ship = ship_cell->ship;
		ship_cell->ship = nullptr;
	}

	delete next_cell;
	delete ship_cell;
}

int GamePrediction::get_index_from_cell(hlt::Position map_position, hlt::GameMap & map, int prediction_step)
{
	int index = prediction_step * (map.height * map.width) + (map_position.y * map.width) + map_position.x;
	return index;
}

int GamePrediction::get_index_from_coordinates(int x, int y, int prediction_step)
{
	int index = prediction_step * (map_width * map_height) + (y * map_width) + x;
	return index;
}

GamePrediction::GamePrediction(Frame & frame, std::unordered_map<hlt::EntityId, Plan> plans, int prediction_steps)
{
	this->prediction_steps = prediction_steps;
	this->map_height = frame.get_game().game_map->height;
	this->map_width = frame.get_game().game_map->width;

	prediction_map = std::make_unique<int[]>((size_t)(frame.get_game().game_map->height * frame.get_game().game_map->width * prediction_steps));
	//prediction_map = std::make_unique<int[]>();
	//Make current state
	for (auto& pair : frame.get_game().me->ships)
	{
		auto id = pair.first;
		auto& ship = pair.second;
		prediction_map[get_index_from_cell(ship->position, *frame.get_game().game_map, 0)] = 1;
	}

	//Make clone of current game state
	GameClone game_clone = GameClone(frame.get_game());

	//Make future state
	for (auto& pair : frame.get_game().me->ships) {
		auto id = pair.first;
		auto& ship = pair.second;

		int depth = 1;
		for (int i = plans[id].execution_step; i < plans[id].path.size(); i++)
		{
			game_clone.advance_game_by_step(plans[id].path[i], *ship, frame);
			hlt::Position ship_at = game_clone.map.at(ship)->position;
			prediction_map[get_index_from_cell(ship_at, *frame.get_game().game_map, depth)] = 1;
			depth++;
		}
		//game_clone.advance_game(plans[ship->id], *ship, frame);
	}
}

std::string GamePrediction::print_prediction()
{
	std::string print_string;
	//for (int p = 0; p < prediction_steps; p++) 
	for (int p = 0; p < 5; p++)
	{
		for(int y = 0; y<map_height; y++)
		{
			for (int x = 0; x<map_width; x++)
			{
				print_string += ((char)(prediction_map[get_index_from_coordinates(x, y, p)]));
			}
			print_string += " ";
		}
		print_string += "\n";
	}
	return std::string();
}
