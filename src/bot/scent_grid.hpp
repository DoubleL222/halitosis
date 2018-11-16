#pragma once

#include "bot/frame.hpp"
#include "hlt/game_map.hpp"

struct ScentOptions {
    int iterations;

    ScentOptions(int iterations);
};

class ScentGrid {
    Frame& frame;
    std::vector<float> scent;

public:
    ScentGrid(Frame& frame, ScentOptions options);

    template <typename T>
    hlt::Direction get_move(hlt::Position pos, T& rng) {
        float scent_sum = 0;
        for (auto dir : ALL_DIRECTIONS) {
            auto new_pos = frame.move(pos, dir);
            scent_sum += scent[frame.get_index(new_pos)];
        }
        float rand = rng();
        float current_sum = 0;
        for (auto dir : ALL_DIRECTIONS) {
            auto new_pos = frame.move(pos, dir);
            current_sum += scent[frame.get_index(new_pos)]/scent_sum;
            if (rand <= current_sum) { return dir; }
        }
        return hlt::Direction::STILL;
    }
};
