#include "bot/frame.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>

Frame::Frame(const hlt::Game& game) : game(game) {}

const hlt::Game& Frame::get_game() const {
    return game;
}

hlt::Position Frame::move(hlt::Position pos, int dx, int dy) {
    pos.x += dx;
    pos.y += dy;
    return game.game_map->normalize(pos);
}

hlt::Position Frame::move(hlt::Position pos, hlt::Direction direction) {
    switch (direction) {
        case hlt::Direction::NORTH: return move(pos, 0, -1);
        case hlt::Direction::EAST: return move(pos, 1, 0);
        case hlt::Direction::SOUTH: return move(pos, 0, 1);
        case hlt::Direction::WEST: return move(pos, -1, 0);
        default: return pos;
    }
}

int get_depth_index(int depth, hlt::GameMap& map, hlt::Position pos) {
    int board_size = map.width*map.height;
    return depth*board_size+pos.y*map.width+pos.x;
}

Path Frame::get_optimal_path(hlt::Ship& ship, hlt::Position end) {
    return get_optimal_path(*game.game_map, ship, end);
}

struct SearchState {
    bool visited;
    hlt::Halite halite;
    std::unordered_map<hlt::Position, hlt::Halite> board_override;
    hlt::Direction in_direction;
};

Path get_search_path(
    hlt::GameMap& map,
    SearchState* search_state,
    hlt::Position start,
    hlt::Position end,
    int max_depth
) {
    Path res(max_depth);
    hlt::Position current_pos = end;
    for (int depth=max_depth; depth>0; depth--) {
        int idx = get_depth_index(depth, map, current_pos);
        res[depth-1] = search_state[idx].in_direction;
        current_pos = current_pos.directional_offset(hlt::invert_direction(res[depth-1]));
        current_pos = map.normalize(current_pos);
    }
    return res;
}

// Find an optimal path to a point for a ship on a specific map.
Path Frame::get_optimal_path(hlt::GameMap& map, hlt::Ship& ship, hlt::Position end) {
    auto start = ship.position;
    int board_size = map.width*map.height;
    int max_depth = std::max(map.width, map.height);
    auto search_state_owned = std::make_unique<SearchState[]>(board_size*max_depth);
    auto search_state = search_state_owned.get();

    int start_idx = get_depth_index(0, map, start);
    search_state[start_idx].halite = ship.halite;
    search_state[start_idx].visited = true;
    search_state[start_idx].board_override = std::unordered_map<hlt::Position, hlt::Halite>();
    for (int depth=0; depth < max_depth-1; depth++) {
        for (int dy=-depth; dy <= depth; dy++) {
            for (int dx=-depth; dx <= depth; dx++) {
                auto pos = move(start, dx, dy);
                int cur_idx = get_depth_index(depth, map, pos);
                if (!search_state[cur_idx].visited) { continue; }

                auto current_halite = search_state[cur_idx].halite;

                int sea_halite = search_state[cur_idx].board_override.count(pos)
                    ? search_state[cur_idx].board_override[pos]
                    : map.at(pos)->halite;
                auto halite_after_move = current_halite-std::ceil(sea_halite*0.1);
                auto halite_after_gather = current_halite;
                if (!map.at(pos)->has_structure()) {
                    halite_after_gather += (hlt::Halite)std::ceil(sea_halite*0.25);
                    halite_after_gather = std::min(halite_after_gather, hlt::constants::MAX_HALITE);
                }

                // Movement
                if (halite_after_move >= 0) {
                    for (auto direction : hlt::ALL_CARDINALS) {
                        int new_idx = get_depth_index(depth+1, map, move(pos, direction));
                        bool update = !search_state[new_idx].visited
                            || halite_after_move > search_state[new_idx].halite;
                        if (update) {
                            search_state[new_idx].halite = halite_after_move;
                            search_state[new_idx].board_override
                                = search_state[cur_idx].board_override;
                            search_state[new_idx].in_direction = direction;
                            search_state[new_idx].visited = true;
                        }
                    }
                }
                // Gathering
                int new_idx = get_depth_index(depth+1, map, pos);
                if (halite_after_gather > search_state[new_idx].halite) {
                    std::unordered_map<hlt::Position, hlt::Halite> new_override(
                        search_state[cur_idx].board_override
                    );
                    new_override[pos] =
                        sea_halite-(halite_after_gather-current_halite);

                    search_state[new_idx].halite = halite_after_gather;
                    search_state[new_idx].board_override = new_override;
                    search_state[new_idx].in_direction = hlt::Direction::STILL;
                    search_state[new_idx].visited = true;
                }
            }
        }
    }
    /*
    for (int depth=0; depth<max_depth; depth++) {
        std::cerr << depth << "-------" << std::endl;
        for (int dy=-depth; dy <= depth; dy++) {
            for (int dx=-depth; dx <= depth; dx++) {
                auto pos = move(start, dx, dy);
                int cur_idx = get_depth_index(depth, map, pos);
                if (search_state[cur_idx].visited) {
                    std::cerr << search_state[cur_idx].halite << " ";
                } else {
                    std::cerr << "X ";

                }
            }
            std::cerr << std::endl;
        }
    }
    */
    float best_halite_per_turn = 0;
    int best_depth = 1;
    for (int depth=1; depth < max_depth; depth++) {
        int idx = get_depth_index(depth, map, end);
        float halite_per_turn = ((float)(search_state[idx].halite))/depth;
        if (halite_per_turn > best_halite_per_turn) {
            best_halite_per_turn = halite_per_turn;
            best_depth = depth;
        }
    }
    return get_search_path(map, search_state, start, end, best_depth);
}
