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

    hlt::Direction next_move() const;
    void advance();
};
