#pragma once

#include "bot/frame.hpp"

struct Plan {
    SearchPath path;
    unsigned int execution_step;
    hlt::Halite final_halite;

    Plan();
    Plan(SearchPath path, hlt::Halite final_halite);

    bool is_finished() const;

    hlt::Halite expected_halite() const;
    // Get the halite expectation after finishing the plan
    hlt::Halite expected_total_halite() const;
    hlt::Direction next_move() const;
    void advance();

    friend std::ostream& operator<<(std::ostream& os, const Plan& grid);
};
