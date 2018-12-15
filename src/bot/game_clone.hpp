#pragma once

#include "bot/bot.hpp"
#include "bot/plan.hpp"
#include "frame.hpp"
#include "hlt/command.hpp"
#include "hlt/game.hpp"

#include <string.h>
#include <random>
#include <vector>

enum SearchPenaltyFactor {
    Zero,
    Decaying,
    One
};

struct SearchState {
    bool visited;
    hlt::Halite halite;
    float penalty;
    int mining_idx;
    std::unordered_map<hlt::Position, int> minings_override;
    hlt::Direction in_direction;
};

class GameClone {
	Frame& frame;
    // Bitset of available minings
    std::vector<unsigned int> minings;
    // Used to mark cells as unsafe once a certain number of turns has passed.
    std::vector<int> turns_until_occupation;
    std::vector<hlt::PlayerId> structures;

public:
	GameClone(Frame& frame);
    // Returns the amount of halite in the ship after executing the plan
    void advance_game(Plan& plan, hlt::Ship& ship);
    void undo_advancement(Plan& plan, hlt::Ship& ship);
    hlt::Halite get_expectation(Plan& plan, hlt::Ship& ship) const;

    // Ensure that optimal minings are used.
    void optimize_minings(std::unordered_map<hlt::EntityId, Plan>& plans) const;

    OptimalPath get_optimal_path(
        hlt::Ship& ship,
        // The number of turns a ship has been underway already.
        // Needed to avoid ships heading home too early
        size_t current_turns_underway,
        SearchPenaltyFactor penalty_factor,
        hlt::Position end,
        time_point end_time,
        int max_depth,
        // Number of turns in which ships should stay closest to own shipyard.
        unsigned int defensive_turns) const;

    // Set that the given position will have all halite removed after a specified number of turns.
    void set_occupied(hlt::Position pos, int turns);

    int width() const;
    int height() const;
    hlt::Halite get_halite(hlt::Position pos, int mining_idx) const;
    bool has_structure(hlt::Position pos) const;
    bool has_own_structure(hlt::Position pos, hlt::PlayerId player) const;
    // Check whether the cell has been occupied after the given number of turns.
    bool is_occupied(hlt::Position pos, int depth) const;

    // Find the cell with the highest halite/distance.
    hlt::Position find_close_halite(hlt::Position start);

private:
    SearchPath get_search_path(
        SearchState* search_state,
        hlt::Position end,
        int max_depth
    ) const;

    // Print a segment of the search states.
    // Prints up to max_depth grids, with the subgrid from center +- grid_size.
    void print_state(SearchState* state, int max_depth, hlt::Position center, int grid_size) const;
};
