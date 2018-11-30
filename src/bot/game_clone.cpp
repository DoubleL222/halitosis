#include "game_clone.hpp"

#include <queue>

GameClone::GameClone(Frame& frame)
  : frame(frame),
    num_minings(frame.get_board_size()),
    turns_until_occupation(frame.get_board_size()),
    structures(frame.get_board_size())
{
    turns_until_occupation.assign(frame.get_board_size(), -1);
    structures.assign(frame.get_board_size(), -1);
    for (auto player : frame.get_game().players) {
        structures[frame.get_index(player->shipyard->position)] = player->id;
        for (auto pair : player->dropoffs) {
            structures[frame.get_index(pair.second->position)] = player->id;
        }
    }
}

// Approximation, as collecting while full leaves some halite on the map
// However, this is easier to reverse.
void GameClone::advance_game(Plan& plan, hlt::Ship& ship) {
    auto current_pos = ship.position;
    for (size_t i = plan.execution_step; i < plan.path.size(); i++) {
        auto move = plan.path[i].direction;
        if (move == hlt::Direction::STILL) {
            auto idx = frame.get_index(current_pos);
            num_minings[idx]++;
        }
        current_pos = frame.move(current_pos, move);
    }
}

void GameClone::undo_advancement(Plan& plan, hlt::Ship& ship) {
    auto current_pos = ship.position;
    for (size_t i = plan.execution_step; i < plan.path.size(); i++) {
        auto move = plan.path[i].direction;
        if (move == hlt::Direction::STILL) {
            auto idx = frame.get_index(current_pos);
            num_minings[idx]--;
        }
        current_pos = frame.move(current_pos, move);
    }
}

void GameClone::set_occupied(hlt::Position pos, int turns) {
    turns_until_occupation[frame.get_index(pos)] = turns;
}

void GameClone::print_state(
    SearchState* search_state,
    int max_depth,
    hlt::Position center,
    int grid_size
) const {
    for (int depth=0; depth < max_depth; depth++) {
        for (int y=-grid_size; y <= grid_size; y++) {
            for (int x=-grid_size; x <= grid_size; x++) {
                auto pos = frame.move(center, x, y);
                auto idx = frame.get_depth_index(depth, pos);
                std::cerr << search_state[idx].visited << " ";
            }
            std::cerr << std::endl;
        }
        std::cerr << std::endl;
    }
}

SearchPath GameClone::get_search_path(
    SearchState* search_state,
    hlt::Position start,
    hlt::Position end,
    int max_depth
) const {
    SearchPath res(max_depth);
    hlt::Position current_pos = end;
    for (int depth=max_depth; depth>0; depth--) {
        int idx = frame.get_depth_index(depth, current_pos);
        auto prev_pos = current_pos.directional_offset(
            hlt::invert_direction(search_state[idx].in_direction));
        prev_pos = frame.get_game().game_map->normalize(prev_pos);
        int prev_idx = frame.get_depth_index(depth-1, current_pos);

        res[depth-1] = PathSegment(
            search_state[idx].in_direction,
            search_state[prev_idx].halite
        );
        current_pos = prev_pos;
    }
    return res;
}

bool should_update_search(SearchState& cur, hlt::Halite halite, SearchState& best) {
    if (!best.visited) { return true; }
    return halite > best.halite;
}

// Find an optimal path to a point for a ship on a specific map.
OptimalPath GameClone::get_optimal_path(
    hlt::Ship& ship,
    hlt::Position end,
    time_point end_time,
    unsigned int max_depth,
    unsigned int defensive_turns
) const {
    auto start = ship.position;
    auto search_state_owned = std::make_unique<SearchState[]>(max_depth*frame.get_board_size());
    auto search_state = search_state_owned.get();

    if (max_depth == 0) {
        OptimalPath res;
        res.path = get_search_path(search_state, start, end, 0);
        res.search_depth = 0;
        return res;
    }

    int start_idx = frame.get_depth_index(0, start);
    search_state[start_idx].halite = ship.halite;
    search_state[start_idx].visited = true;
    search_state[start_idx].board_override = std::unordered_map<hlt::Position, hlt::Halite>();
    unsigned int search_depth = 0;
    for (
        auto now = ms_clock::now();
        search_depth < max_depth-1 && now < end_time;
        now = ms_clock::now(), search_depth++
    ) {
        unsigned int current_turn = frame.get_game().turn_number+search_depth;
        int search_dist_x = std::min(search_depth, static_cast<unsigned int>(width())/2);
        int search_dist_y = std::min(search_depth, static_cast<unsigned int>(height())/2);
        for (int dy=-search_dist_y; dy <= search_dist_y; dy++) {
            for (int dx=-search_dist_x; dx <= search_dist_x; dx++) {
                auto pos = frame.move(start, dx, dy);
                int cur_idx = frame.get_depth_index(search_depth, pos);
                if (!search_state[cur_idx].visited) { continue; }

                auto current_halite = search_state[cur_idx].halite;

                int sea_halite = 0;
                if (!is_occupied(pos, search_depth) && !has_structure(pos)) {
                    if (search_state[cur_idx].board_override.count(pos)) {
                        sea_halite = search_state[cur_idx].board_override[pos];
                    } else {
                        sea_halite = get_halite(pos);
                    }
                }
                auto halite_after_move = current_halite-std::ceil(sea_halite*0.1);
                auto halite_after_gather = current_halite;
                if (!has_structure(pos)) {
                    halite_after_gather += (hlt::Halite)std::ceil(sea_halite*0.25);
                    halite_after_gather = std::min(halite_after_gather, hlt::constants::MAX_HALITE);
                }

                // Movement
                if (halite_after_move >= 0) {
                    for (auto direction : hlt::ALL_CARDINALS) {
                        auto new_pos = frame.move(pos, direction);
                        auto my_id = frame.get_game().my_id;
                        bool defensive_move = (frame.get_closest_shipyard(new_pos) == my_id);
                        if (current_turn >= defensive_turns || defensive_move) {
                            int new_idx = frame.get_depth_index(search_depth+1, new_pos);

                            bool update = should_update_search(
                                search_state[cur_idx],
                                halite_after_move,
                                search_state[new_idx]);
                            if (update) {
                                search_state[new_idx].board_override
                                    = search_state[cur_idx].board_override;
                                search_state[new_idx].in_direction = direction;
                                search_state[new_idx].visited = true;
                                search_state[new_idx].halite = halite_after_move;
                            }
                        }
                    }
                }
                // Gathering
                int new_idx = frame.get_depth_index(search_depth+1, pos);
                bool update = should_update_search(
                    search_state[cur_idx],
                    halite_after_gather,
                    search_state[new_idx]);
                if (update) {
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

    int best_per_turn_depth = 0;
    float best_halite_per_turn = 0;
    for (unsigned int depth=1; depth < search_depth; depth++) {
        int idx = frame.get_depth_index(depth, end);
        float halite_per_turn = ((float)(search_state[idx].halite))/depth;
        if (halite_per_turn > best_halite_per_turn) {
            best_halite_per_turn = halite_per_turn;
            best_per_turn_depth = depth;
        }
    }
    OptimalPath res;
    res.path = get_search_path(search_state, start, end, best_per_turn_depth);
    res.search_depth = search_depth;
    return res;
}

int GameClone::width() const {
    return frame.get_game().game_map->width;
}

int GameClone::height() const {
    return frame.get_game().game_map->height;
}

hlt::Halite GameClone::get_halite(hlt::Position pos) const {
    auto res = frame.get_game().game_map->at(pos)->halite;
    auto idx = frame.get_index(pos);
    for (unsigned int i=0; i < num_minings[idx]; i++) {
        res -= res/hlt::constants::EXTRACT_RATIO;
    }
    return res;
}

bool GameClone::has_structure(hlt::Position pos) const {
    return structures[frame.get_index(pos)] != -1;
}

bool GameClone::has_own_structure(hlt::Position pos, hlt::PlayerId player) const {
    return structures[frame.get_index(pos)] == player;
}

bool GameClone::is_occupied(hlt::Position pos, int depth) const {
    auto idx = frame.get_index(pos);
    return turns_until_occupation[idx] != -1 && turns_until_occupation[idx] <= depth;
}

hlt::Position GameClone::find_close_halite(hlt::Position start) {
    auto visited = std::vector<bool>(frame.get_board_size());

    hlt::Position best(0, 0);
    float best_per_turn = 0;

    std::queue<hlt::Position> queue;
    queue.push(start);
    while (!queue.empty()) {
        auto pos = queue.front();
        queue.pop();

        auto idx = frame.get_index(pos);
        if (!visited[idx]) {
            visited[idx] = true;
            auto dist = frame.get_game().game_map->calculate_distance(start, pos);
            float halite = 0;
            // Set to high value, as opponents ships should not compete amongst themselves
            if (is_occupied(pos, 200)) {
                halite = get_halite(pos);
            }
            auto per_turn = halite/dist;
            if (per_turn > best_per_turn) {
                best_per_turn = per_turn;
                best = pos;
            }
            for (auto dir : hlt::ALL_CARDINALS) {
                queue.push(frame.move(pos, dir));
            }
        }
    }
    return best;
}
