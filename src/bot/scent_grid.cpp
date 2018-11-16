#include "bot/scent_grid.hpp"

ScentOptions::ScentOptions(int iterations)
  : iterations(iterations)
{
}

ScentGrid::ScentGrid(Frame& frame, ScentOptions options)
  : frame(frame)
{
    auto& map = frame.get_game().game_map;
    scent = std::vector<float>(map->width*map->height);
    for (int iteration=0; iteration < options.iterations; iteration++) {
        std::vector<float> new_scent(scent);
        // Propagate scent
        for (int y=0; y < map->height; y++) {
            for (int x=0; x < map->width; x++) {
                hlt::Position pos(x, y);
                for (auto dir : ALL_DIRECTIONS) {
                    auto new_pos = frame.move(pos, dir);
                    new_scent[frame.get_index(new_pos)] += scent[frame.get_index(pos)]/5;
                }
            }
        }

        // Add scent
        for (int y=0; y < map->height; y++) {
            for (int x=0; x < map->width; x++) {
                hlt::Position pos(x, y);
                new_scent[frame.get_index(pos)] += map->at(pos)->halite;
            }
        }
        scent = new_scent;
    }
}
