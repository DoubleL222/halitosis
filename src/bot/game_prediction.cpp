#include "game_prediction.hpp"

GameClone::GameClone(const hlt::Game & game)
{
	this->map = hlt::GameMap(*game.game_map);
}

std::vector<hlt::Position> GameClone::advance_game(Plan & plan, hlt::Ship & ship, Frame & frame)
{
	//for (int i = plan.execution_step; i<plan.path.size(); i++)
	//{
	//	hlt::MapCell * ship_cell = map.at(ship);

	//	if (plan.path[i] == hlt::Direction::STILL)
	//	{
	//		//Mine The Cell
	//		ship_cell->halite = ship_cell->halite * 0.75f;
	//	}
	//	else
	//	{
	//		hlt::Position next_pos = frame.move(ship.position, plan.path[i]);
	//		hlt::MapCell * next_cell = map.at(next_pos);
	//		next_cell->ship = ship_cell->ship;
	//		ship_cell->ship = nullptr;
	//	}
	//}
	std::vector<hlt::Position> all_positions;
	all_positions.push_back(ship.position);
	int position_iterator = 0;
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
		all_positions.push_back(advance_game_by_step(plan.path[i], all_positions[position_iterator], frame));
		position_iterator++;
	}
	return all_positions;
}

hlt::Position GameClone::advance_game_by_step(hlt::Direction dir, hlt::Position current_position, Frame & frame)
{
	hlt::MapCell * ship_cell;
	hlt::MapCell * next_cell;

	ship_cell = map.at(current_position);

	if (dir == hlt::Direction::STILL)
	{
		//Mine The Cell
		ship_cell->halite = ship_cell->halite * 0.75f;

		return ship_cell->position;
	}
	else
	{
		//move
		hlt::Position next_pos = frame.move(current_position, dir);
		next_cell = map.at(next_pos);
		next_cell->ship = ship_cell->ship;
		ship_cell->ship = nullptr;

		return next_cell->position;
	}

	//delete next_cell;
	//delete ship_cell;
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
	/*for (auto& pair : frame.get_game().me->ships)
	{
		auto id = pair.first;
		auto& ship = pair.second;
		prediction_map[get_index_from_cell(ship->position, *frame.get_game().game_map, 0)] = 1;
	}*/

	//Make clone of current game state
	GameClone game_clone = GameClone(frame.get_game());

	//Make future state
	for (auto& pair : frame.get_game().me->ships) {
		auto id = pair.first;
		auto& ship = pair.second;
		std::vector<hlt::Position> all_positions = game_clone.advance_game(plans[id], *ship, frame);
		
		int depth = 0;
		for (hlt::Position next_position : all_positions) 
		{
			prediction_map[get_index_from_cell(next_position, *frame.get_game().game_map, depth)] = 1;
			depth = depth + 1;
		}
		//for (int i = plans[id].execution_step; i < plans[id].path.size(); i++)
		//{
		//	//game_clone.advance_game_by_step(plans[id].path[i], *ship, frame);
		//	//hlt::Position ship_at = game_clone.map.at(ship)->position;
		//	//hlt::Position ship_at = game_clone.advance_game_by_step(plans[id].path[i], *ship, frame);
		//	
		//	prediction_map[get_index_from_cell(ship_at, *frame.get_game().game_map, depth)] = 1;
		//	depth = depth+1;
		//}
		//game_clone.advance_game(plans[ship->id], *ship, frame);
	}
}

std::string GamePrediction::print_prediction()
{
	std::string print_string;
	print_string += "PRINT PREDICTION: \n";
	//for (int p = 0; p < prediction_steps; p++) 
	for (int p = 0; p < 10; p++)
	{
		print_string += "    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 \n";
		for(int y = 0; y<map_height; y++)
		{
			print_string += std::to_string(y);
			if (y < 10) 
			{
				print_string += " ";
			}
			print_string += ": ";
			for (int x = 0; x<map_width; x++)
			{
				print_string += std::to_string(prediction_map[get_index_from_coordinates(x, y, p)]);
				print_string += "  ";
				//print_string += prediction_map[get_index_from_coordinates(x, y, p)];
			}
			print_string += "\n";
		}
		print_string += "\n \n Prediction: \n";
	}
	return print_string;
}
