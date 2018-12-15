#include "game_clone.hpp"
#include "bot/math.hpp"

#include <queue>

GameClone::GameClone(Frame& frame)
  : frame(frame),
    minings(frame.get_board_size()),
    turns_until_occupation(frame.get_board_size()),
    structures(frame.get_board_size())
{
    minings.assign(frame.get_board_size(), (1 << 25)-1);
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
            minings[idx] ^= (1 << plan.path[i].mining_idx);
        }
        current_pos = frame.move(current_pos, move);
    }
}

void GameClone::undo_advancement(Plan& plan, hlt::Ship& ship) {
    // advance_game is currently a toggle.
    advance_game(plan, ship);
}

hlt::Halite GameClone::get_expectation(Plan& plan, hlt::Ship& ship) const {
    hlt::Halite res = ship.halite;
    hlt::Position pos = ship.position;
    for (size_t i=plan.execution_step; i < plan.path.size(); i++) {
        auto halite = get_halite(pos, plan.path[i].mining_idx);
        if (plan.path[i].direction == hlt::Direction::STILL) {
            res += ceil_div(halite, hlt::constants::EXTRACT_RATIO);
        } else {
            res -= halite/hlt::constants::MOVE_COST_RATIO;
        }
        pos = frame.move(pos, plan.path[i].direction);
    }
    return res;
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
    hlt::Position end,
    int max_depth
) const {
    SearchPath res(max_depth);
    hlt::Position current_pos = end;
    for (int depth=max_depth; depth>0; depth--) {
        int idx = frame.get_depth_index(depth, current_pos);
        auto prev_pos =
            frame.move(current_pos, hlt::invert_direction(search_state[idx].in_direction));
        int prev_idx = frame.get_depth_index(depth-1, prev_pos);

        res[depth-1] = PathSegment(
            search_state[idx].in_direction,
            search_state[prev_idx].halite,
            search_state[idx].mining_idx
        );
        current_pos = prev_pos;
    }
    return res;
}

bool should_update_search(SearchState& cur, hlt::Halite halite, float penalty, SearchState& best) {
    if (!best.visited) { return true; }
    return halite-cur.penalty-penalty > best.halite-best.penalty;
}

int get_least_significant_one_idx(int bits) {
    int res = 0;
    int probe = 1;
    while ((probe & bits) == 0) {
        res++;
        probe <<= 1;
    }
    return res;
}

// Find an optimal path to a point for a ship on a specific map.
OptimalPath GameClone::get_optimal_path(
    hlt::Ship& ship,
    size_t current_turns_underway,
    SearchPenaltyFactor penalty_factor,
    hlt::Position end,
    time_point end_time,
    int max_depth,
    unsigned int defensive_turns
) const {
    auto start = ship.position;
    // Avoid segfault when max_depth == 0
    auto search_state_depth = std::max(max_depth, 1);
    auto search_state_owned = std::make_unique<SearchState[]>(search_state_depth*frame.get_board_size());
    auto search_state = search_state_owned.get();

    int start_idx = frame.get_depth_index(0, start);
    search_state[start_idx].halite = ship.halite;
    search_state[start_idx].penalty = 0;
    search_state[start_idx].visited = true;
    search_state[start_idx].minings_override = std::unordered_map<hlt::Position, int>();
    int search_depth = 0;
    for (
        auto now = ms_clock::now();
        search_depth < max_depth-1 && now < end_time;
        now = ms_clock::now(), search_depth++
    ) {
        unsigned int current_turn = frame.get_game().turn_number+search_depth;
        int search_dist_x = std::min(search_depth, width()/2);
        int search_dist_y = std::min(search_depth, height()/2);
        for (int dy=-search_dist_y; dy <= search_dist_y; dy++) {
            for (int dx=-search_dist_x; dx <= search_dist_x; dx++) {
                auto pos = frame.move(start, dx, dy);
                int cur_idx = frame.get_depth_index(search_depth, pos);
                if (!search_state[cur_idx].visited) { continue; }

                auto current_halite = search_state[cur_idx].halite;

                int available_minings = minings[frame.get_index(pos)];
                if (search_state[cur_idx].minings_override.count(pos)) {
                    available_minings = search_state[cur_idx].minings_override[pos];
                }

                bool mining_possible = available_minings != 0 && !has_structure(pos);
                auto next_mining_idx =
                    mining_possible ? get_least_significant_one_idx(available_minings) : 0;
                auto sea_halite = mining_possible ? get_halite(pos, next_mining_idx) : 0;

                auto move_cost = sea_halite/hlt::constants::MOVE_COST_RATIO;
                auto halite_after_move = current_halite-move_cost;
                auto halite_after_gather =
                    current_halite+ceil_div(sea_halite, hlt::constants::EXTRACT_RATIO);
                halite_after_gather = std::min(hlt::constants::MAX_HALITE, halite_after_gather);
                float penalty = 0;
                switch (penalty_factor) {
                    case SearchPenaltyFactor::One:
                        penalty = move_cost;
                        break;
                    case SearchPenaltyFactor::Decaying:
                        penalty = move_cost*(1.0-current_turn/hlt::constants::MAX_TURNS);
                        break;
                    case SearchPenaltyFactor::Zero:
                        break;
                }

                // Movement
                if (halite_after_move >= 0) {
                    for (auto direction : hlt::ALL_CARDINALS) {
                        auto new_pos = frame.move(pos, direction);
                        if (search_depth != 0 || !frame.ship_at(new_pos)) {
                            auto my_id = frame.get_game().my_id;
                            bool defensive_move = (frame.get_closest_shipyard(new_pos) == my_id);
                            if (current_turn >= defensive_turns || defensive_move) {
                                int new_idx = frame.get_depth_index(search_depth+1, new_pos);

                                bool update = should_update_search(
                                    search_state[cur_idx],
                                    halite_after_move,
                                    penalty,
                                    search_state[new_idx]);
                                if (update) {
                                    search_state[new_idx].minings_override
                                        = search_state[cur_idx].minings_override;
                                    // Matters, as get_expectation uses it to compute the cost of moving
                                    search_state[new_idx].mining_idx =
                                        mining_possible ? next_mining_idx : 30;
                                    search_state[new_idx].in_direction = direction;
                                    search_state[new_idx].visited = true;
                                    search_state[new_idx].halite = halite_after_move;
                                    search_state[new_idx].penalty =
                                        search_state[cur_idx].penalty+move_cost;
                                }
                            }
                        }
                    }
                }
                // Gathering
                int new_idx = frame.get_depth_index(search_depth+1, pos);
                bool update = mining_possible && should_update_search(
                    search_state[cur_idx],
                    halite_after_gather,
                    0,
                    search_state[new_idx]);
                if (update) {
                    std::unordered_map<hlt::Position, int> new_override(
                        search_state[cur_idx].minings_override
                    );
                    new_override[pos] = (available_minings ^ (1 << next_mining_idx));

                    search_state[new_idx].halite = halite_after_gather;
                    search_state[new_idx].penalty = search_state[cur_idx].penalty;
                    search_state[new_idx].minings_override = new_override;
                    search_state[new_idx].mining_idx = next_mining_idx;
                    search_state[new_idx].in_direction = hlt::Direction::STILL;
                    search_state[new_idx].visited = true;
                }
            }
        }
    }

    int best_per_turn_depth = 0;
    float best_halite_per_turn = 0;
    hlt::Halite best_halite = 0;
    for (int depth=1; depth < search_depth; depth++) {
        int idx = frame.get_depth_index(depth, end);
        float score = search_state[idx].halite-search_state[idx].penalty;
        float halite_per_turn = score/(depth+current_turns_underway);
        if (halite_per_turn > best_halite_per_turn) {
            best_halite_per_turn = halite_per_turn;
            best_per_turn_depth = depth;
            best_halite = search_state[idx].halite;
        }
    }

    OptimalPath res;
    res.search_depth = search_depth;
    res.final_halite = best_halite;
    if (best_per_turn_depth == 0) {
        // No path found
        res.path = {};
    } else {
        res.path = get_search_path(search_state, end, best_per_turn_depth);
    }
    return res;
}

int GameClone::width() const {
    return frame.get_game().game_map->width;
}

int GameClone::height() const {
    return frame.get_game().game_map->height;
}

hlt::Halite GameClone::get_halite(hlt::Position pos, int mining_idx) const {
    auto res = frame.get_game().game_map->at(pos)->halite;
    for (int i=0; i < mining_idx; i++) {
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
                halite = get_halite(pos, 0);
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
