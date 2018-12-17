#pragma once

#include <random>
#include "bot/frame.hpp"
#include "hlt/game_map.hpp"

struct OmniDirectionalValue {
    float up;
    float right;
    float down;
    float left;

    friend std::ostream& operator<<(std::ostream& os, const OmniDirectionalValue& grid);
};
std::ostream& operator<<(std::ostream& os, const OmniDirectionalValue& val);

class GravityGrid {
    int width;
    int height;
    std::vector<float> attractors;
    std::vector<OmniDirectionalValue> pull;

public:
    GravityGrid(int width, int height);
    // Add gravity to the previous value
    void add_gravity(int x, int y, float gravity);
    // Reset gravity to a new value for a position.
    // Is not recomputed if the gravity stays the same.
    void set_gravity(int x, int y, float gravity);

    // position is represented as an index, move is an index into the ALL_MOVES array.
    float get_pull(int position, size_t move) const;
    friend std::ostream& operator<<(std::ostream& os, const GravityGrid& grid);
};

std::ostream& operator<<(std::ostream& os, const GravityGrid& grid);
