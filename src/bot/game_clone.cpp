#include "game_clone.hpp"

hlt::GameMap& GameClone::get_map()
{
	return map;
}

// use this to check if cell will be occupied
// prediction_step is how many turns in the future we're looking in ( 0 == current turn)
bool GameClone::is_cell_occupied(int x, int y, int prediction_step)
{
	int index = prediction_step * (map.height * map.width) + (y * map.width) + x;
	return index;
}

// use this to check if cell will be occupied
// prediction_step is how many turns in the future we're looking in ( 0 == current turn)
bool GameClone::is_cell_occupied(hlt::Position map_position, int prediction_step)
{
	int index = prediction_step * (map.height * map.width) + (map_position.y * map.width) + map_position.x;
	return index;
}

GameClone::GameClone(Frame * frame)
{
	this->map = hlt::GameMap(*frame->get_game().game_map);
	this->frame = frame;
	this->do_prediction = false;
}

GameClone::GameClone(Frame * frame, bool do_prediction, int prediction_steps)
{
	this->map = hlt::GameMap(*frame->get_game().game_map);
	this->frame = frame;
	this->do_prediction = do_prediction;
	this->prediction_steps = prediction_steps;
	if (do_prediction) 
	{
		prediction_map = std::make_unique<int[]>((size_t)(frame->get_game().game_map->height * frame->get_game().game_map->width * prediction_steps));
		//Make clone of current game state
		//GameClone game_clone = GameClone(frame->get_game());

		////Make future state
		//for (auto& pair : frame->get_game().me->ships) {
		//	auto id = pair.first;
		//	auto& ship = pair.second;
		//	std::vector<hlt::Position> all_positions = game_clone.advance_game(plans[id], *ship);

		//	int depth = 0;
		//	for (hlt::Position next_position : all_positions)
		//	{
		//		prediction_map[get_index_from_cell(next_position, *frame->get_game().game_map, depth)] = 1;
		//		depth = depth + 1;
		//	}
		//}
	}
}

std::vector<hlt::Position> GameClone::advance_game(Plan & plan, hlt::Ship & ship)
{
	std::vector<hlt::Position> all_positions;
	all_positions.push_back(ship.position);
	int position_iterator = 0;
	for (int i = plan.execution_step; i < plan.path.size(); i++)
	{
		all_positions.push_back(advance_game_by_step(plan.path[i], all_positions[position_iterator]));
		position_iterator++;
	}

	if (this->do_prediction) 
	{
		int depth = 0;
		for (hlt::Position next_position : all_positions)
		{
			prediction_map[get_index_from_cell(next_position, depth)] = 1;
			depth = depth + 1;
		}
	}

	return all_positions;
}

hlt::Position GameClone::advance_game_by_step(hlt::Direction dir, hlt::Position current_position)
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
		hlt::Position next_pos = frame->move(current_position, dir);
		next_cell = map.at(next_pos);
		next_cell->ship = ship_cell->ship;
		ship_cell->ship = nullptr;

		return next_cell->position;
	}
}

int GameClone::get_index_from_coordinates(int x, int y, int prediction_step)
{
	int index = prediction_step * (map.height * map.width) + (y * map.width) + x;
	return index;
}

int GameClone::get_index_from_cell(hlt::Position map_position, int prediction_step)
{
	int index = prediction_step * (map.height * map.width) + (map_position.y * map.width) + map_position.x;
	return index;
}

std::string GameClone::print_prediction()
{
	std::string print_string;
	print_string += "PRINT PREDICTION: \n";
	//for (int p = 0; p < prediction_steps; p++) 
	for (int p = 0; p < 10; p++)
	{
		print_string += "    0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 \n";
		for (int y = 0; y<map.height; y++)
		{
			print_string += std::to_string(y);
			if (y < 10)
			{
				print_string += " ";
			}
			print_string += ": ";
			for (int x = 0; x<map.width; x++)
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
