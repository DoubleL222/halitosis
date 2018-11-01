#pragma once

#include "bot/bot.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"
#include "first.hpp"

#include <string.h>
#include <random>
#include <vector>

class GamePrediction
{
	std::unique_ptr<int[]> prediction_map;
	int get_index_from_cell(hlt::Position map_position, hlt::GameMap & map, int prediction_step);
	int get_index_from_coordinates(int x, int y, int prediction_step);
	int prediction_steps;
	int map_width;
	int map_height;

public:
	GamePrediction(Frame & frame, std::unordered_map<hlt::EntityId, Plan> plans, int prediction_steps);
	std::string print_prediction();
};

struct GameClone
{
	GameClone(const hlt::Game & game);

	hlt::GameMap map;

	void advance_game(Plan & plan, hlt::Ship & ship, Frame & frame);
	void advance_game_by_step(hlt::Direction dir, hlt::Ship & ship, Frame & frame);
};