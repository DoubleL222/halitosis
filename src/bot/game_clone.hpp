#pragma once

#include "bot/bot.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"
#include "first.hpp"

#include <string.h>
#include <random>
#include <vector>

class GameClone
{
private:
	Frame * frame;
	bool do_prediction;
	int prediction_steps;
	hlt::GameMap map;
	std::unique_ptr<int[]> prediction_map;
	hlt::Position advance_game_by_step(hlt::Direction dir, hlt::Position current_position);
	int get_index_from_coordinates(int x, int y, int prediction_step);
	int get_index_from_cell(hlt::Position map_position, int prediction_step);

public:
	hlt::GameMap& get_map();
	// use this to check if cell will be occupied
	// prediction_step is how many turns in the future we're looking in ( 0 = current turn)
	bool is_cell_occupied(int x, int y, int prediction_step);
	bool is_cell_occupied(hlt::Position map_position, int prediction_step);
	GameClone(Frame * frame);
	GameClone(hlt::GameMap * game_map);
	GameClone(Frame * frame, bool do_prediction, int prediction_steps);
	std::vector<hlt::Position> advance_game(Plan & plan, hlt::Ship & ship);
	std::string print_prediction();
};
