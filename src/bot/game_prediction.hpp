#pragma once

#include "bot/bot.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"
#include "first.hpp"

#include <random>
#include <vector>

class GamePrediction
{
	std::unique_ptr<int[]> prediction_map;
public:
	GamePrediction(const hlt::Game & game, std::unordered_map<hlt::EntityId, Plan> plans, int prediction_steps);
};

struct GameClone
{
	GameClone(const hlt::Game & game);

	hlt::GameMap map;

	void advance_game(Plan & plan, hlt::Ship & ship, Frame & frame);
};