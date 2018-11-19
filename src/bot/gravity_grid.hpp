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

const int RETURN_FACTOR = 10;
const int MINE_FACTOR = 10;
class GravityGrid {
    Frame& frame;
    std::vector<float> attractors;
    std::vector<OmniDirectionalValue> pull;
    std::vector<OmniDirectionalValue> return_pull;

public:
    GravityGrid(Frame& frame);

    template <typename T>
    hlt::Direction get_move(hlt::Position pos, hlt::Halite current_halite, T& rng) {
        float pull_fraction =1.0;
            //((float)hlt::constants::MAX_HALITE-current_halite)/hlt::constants::MAX_HALITE;
        float return_fraction = 0.0;//1.0-pull_fraction;

        auto idx = frame.get_index(pos);
        float pull_sum = MINE_FACTOR*attractors[idx]*pull_fraction;
        pull_sum += pull[idx].left*pull_fraction+return_pull[idx].left*return_fraction;
        pull_sum += pull[idx].right*pull_fraction+return_pull[idx].right*return_fraction;
        pull_sum += pull[idx].up*pull_fraction+return_pull[idx].up*return_fraction;
        pull_sum += pull[idx].down*pull_fraction+return_pull[idx].down*return_fraction;

        std::uniform_real_distribution<float> distribution{};
        float rand = distribution(rng);

        float current_sum = MINE_FACTOR*attractors[idx]/pull_sum*pull_fraction;
        if (rand <= current_sum) return hlt::Direction::STILL;
        current_sum += (pull[idx].left*pull_fraction+return_pull[idx].left*return_fraction)/pull_sum;
        if (rand <= current_sum) return hlt::Direction::WEST;
        current_sum += (pull[idx].right*pull_fraction+return_pull[idx].right*return_fraction)/pull_sum;
        if (rand <= current_sum) return hlt::Direction::EAST;
        current_sum += (pull[idx].up*pull_fraction+return_pull[idx].up*return_fraction)/pull_sum;
        if (rand <= current_sum) return hlt::Direction::NORTH;
        current_sum += (pull[idx].down*pull_fraction+return_pull[idx].down*return_fraction)/pull_sum;
        if (rand <= current_sum) return hlt::Direction::SOUTH;
        return hlt::Direction::STILL;
    }
    friend std::ostream& operator<<(std::ostream& os, const GravityGrid& grid);
};

std::ostream& operator<<(std::ostream& os, const GravityGrid& grid);
