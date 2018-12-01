#pragma once

#include "bot/frame.hpp"

struct Plan {
    SearchPath path;
    unsigned int execution_step;

    Plan();
    Plan(SearchPath path);
    // Create a plan with all expectations set to -1.
    Plan(Path path);

    bool is_finished() const;

    hlt::Halite expected_halite() const;
    // Get the halite expectation after finishing the plan
    hlt::Halite expected_total_halite() const;
    hlt::Direction next_move() const;
    void advance();

    friend std::ostream& operator<<(std::ostream& os, const Plan& grid);
};
