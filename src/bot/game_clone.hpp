#pragma once

#include "bot/bot.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"
#include "first.hpp"

#include <string.h>
#include <random>
#include <vector>

struct SearchState {
    bool visited;
    hlt::Halite halite;
    std::unordered_map<hlt::Position, hlt::Halite> board_override;
    hlt::Direction in_direction;
};

class GameClone {
	Frame& frame;
    std::vector<hlt::Halite> halite;
    // Used to mark cells as unsafe once a certain number of turns has passed.
    std::vector<int> turns_until_occupation;
    std::vector<hlt::PlayerId> structures;

public:
	GameClone(Frame& frame);
	void advance_game(Plan& plan, hlt::Ship& ship);

    OptimalPath get_optimal_path(
        hlt::Ship& ship,
        hlt::Position end,
        time_point end_time,
        unsigned int max_depth,
        // Number of turns in which ships should stay closest to own shipyard.
        unsigned int defensive_turns) const;

    // Set that the given position will have all halite removed after a specified number of turns.
    void set_occupied(hlt::Position pos, int turns);

    int width() const;
    int height() const;
    hlt::Halite get_halite(hlt::Position pos) const;
    bool has_structure(hlt::Position pos) const;
    // Check whether the cell has been occupied after the given number of turns.
    bool is_occupied(hlt::Position pos, int depth) const;

    // Find the cell with the highest halite/distance.
    hlt::Position find_close_halite(hlt::Position start);

private:
    SearchPath get_search_path(
        SearchState* search_state,
        hlt::Position start,
        hlt::Position end,
        int max_depth
    ) const;
};
