#include "bot/math.hpp"
#include "bot/mcts.hpp"
#include "bot/frame.hpp"

#include <cmath>
#include <random>

//#define DEBUG

// Larger values will increase exploration, smaller will increase exploitation.
const float EXPLORATION_CONSTANT = std::sqrt(2.0);

// The maximum depth that moves will be generated for
const int MAX_DEPTH = 50;

// Number of simulations used to generate a score to beat when we don't have one from the previous
// turn
const int NUM_INIT_SIMULATIONS = 10;

// A container for moves that allows faster access than a vec of vecs
struct ShipMoves {
    int num_ships;
    std::vector<int> num_moves;
    std::vector<int> moves;

    ShipMoves(int num_ships)
      : num_ships(num_ships),
        num_moves(num_ships),
        moves(num_ships*(MAX_DEPTH+1))
    {
    }

    int get_path_size(int ship_idx) const {
        return num_moves[ship_idx];
    }

    bool is_move_specified(int ship_idx, int depth) const {
        return depth < num_moves[ship_idx];
    }

    int get_move(int ship_idx, int depth) const {
        return moves[depth*num_ships+ship_idx];
    }

    void push_bounded(int ship_idx, int move, int bound) {
        int depth = num_moves[ship_idx];
        if (depth < bound) {
            moves[depth*num_ships+ship_idx] = move;
            num_moves[ship_idx]++;
        }
    }

    void push(int ship_idx, int move) {
        push_bounded(ship_idx, move, MAX_DEPTH);
    }

    void push_temp(int ship_idx, int move) {
        push_bounded(ship_idx, move, MAX_DEPTH+1);
    }

    void pop(int ship_idx) {
        num_moves[ship_idx]--;
    }

    void clear(int ship_idx) {
        num_moves[ship_idx] = 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const ShipMoves& moves);
};

std::ostream& operator<<(std::ostream& os, const ShipMoves& moves) {
    os << "{";
    for (int ship_idx=0; ship_idx < moves.num_ships; ship_idx++) {
        os << (ship_idx == 0 ? " " : ", ") << "[";
        for (int depth=0; moves.is_move_specified(ship_idx, depth); depth++) {
            os << (depth == 0 ? " " : ", ");
            os << moves.get_move(ship_idx, depth);
        }
        os << " ]";
    }
    os << " }";
    return os;
}

struct MctsTreeNode {
    int visits;
    float total_reward;
    std::vector<std::unique_ptr<MctsTreeNode>> children;

    MctsTreeNode()
      : visits(0),
        total_reward(0)
    {
    }

    bool is_expanded() const {
        return !children.empty();
    }

    hlt::Direction best_move() {
        int best_child = 0;
        int best_visits = 0;
        float best_total_reward = 0;
        for (unsigned int child_idx = 0; child_idx < ALL_DIRECTIONS.size(); child_idx++) {
            if (children[child_idx]->visits > best_visits) {
                best_visits = children[child_idx]->visits;
                best_child = child_idx;
                best_total_reward = children[child_idx]->total_reward;
            }
        }
#ifdef DEBUG
        std::cerr << "best_move: " << best_child << ": " << best_total_reward << "/" << best_visits << std::endl;
#endif
        return ALL_DIRECTIONS[best_child];
    }

    int get_best_child() {
        float best_score = -1.0;
        int best_child = 0;
        for (unsigned int child_idx = 0; child_idx < ALL_DIRECTIONS.size(); child_idx++) {
            auto& child = *children[child_idx];
            float exploit = ((float)child.total_reward)/child.visits;
            // Decreases when child is visited
            float explore = std::sqrt(std::log(visits)/child.visits);
            float score = exploit+EXPLORATION_CONSTANT*explore;
            // TODO why random choice when about equal?
            if (score > best_score) {
                best_child = child_idx;
                best_score = score;
            }
        }
        return best_child;
    }

    // function TREEPOLICY(v)
    //     while v is nonterminal do
    //         if v not fully expanded then
    //             return EXPAND(v)
    //         else
    //             v ← BESTCHILD(v, Cp)
    //     return v
    //
    void tree_policy(ShipMoves& moves, int ship_idx) {
        if (is_expanded()) {
            int best_child = get_best_child();
            moves.push(ship_idx, best_child);
            if (!moves.is_move_specified(ship_idx, MAX_DEPTH-1)) {
                children[best_child]->tree_policy(moves, ship_idx);
            }
        } else {
            // Expand
            for (size_t move = 0; move < ALL_DIRECTIONS.size(); move++) {
                children.push_back(std::make_unique<MctsTreeNode>());
            }
        }
    }

    void update_rec(const ShipMoves& moves, int depth, int ship_idx, float reward) {
        visits++;
        total_reward += reward;
        if (moves.is_move_specified(ship_idx, depth)) {
            children[moves.get_move(ship_idx, depth)]->update_rec(moves, depth+1, ship_idx, reward);
        }
    }

    void update(const ShipMoves& moves, int ship_idx, float reward) {
        update_rec(moves, 0, ship_idx, reward);
    }

    friend std::ostream& operator<<(std::ostream& os, const MctsTreeNode& node);
};

std::ostream& operator<<(std::ostream& os, const MctsTreeNode& node) {
    os << "{ " << node.total_reward << "/" << node.visits;
    if (node.is_expanded()) {
        os << " [";
        for (auto& child : node.children) {
            os << " " << *child;
        }
        os << " ]";
    }
    os << " }";
    return os;
}

struct MctsTree {
    // Scores to compare against. A score > this value is counted as a win, while less than this is
    // a draw.
    // Usually the previous average scores are compared against.
    float comparison_score;
    // Sum of current scores to compute the average.
    float current_score_sum;

    MctsTreeNode root;

    MctsTree(float comparison_score)
      : comparison_score(comparison_score),
        current_score_sum(0)
    {
    }

    // Get the best move found by the mcts
    hlt::Direction best_move() {
        return root.best_move();
    }

    // Sets the path in the moves struct instead of returning a newly allocated path.
    void tree_policy(ShipMoves& moves, int ship_idx) {
        moves.clear(ship_idx);
        root.tree_policy(moves, ship_idx);
    }

    // Update the tree using a score that does not need to be normalized.
    void update(const ShipMoves& moves, int ship_idx, float score) {
        current_score_sum += score;
        float reward = 0.5;
        if (score > comparison_score) { reward = 1.0; }
        if (score < comparison_score) { reward = 0.0; }
        root.update(moves, ship_idx, reward);
    }

    // Retrieve the average score that the ship has gained across all simulations.
    float get_average_score() {
        return current_score_sum/root.visits;
    }
};

struct SimulatedShip {
    int position;
    hlt::Halite halite;
    int turns_underway;
    // Set to -1 if the dropoff has not been reached yet.
    float halite_per_turn;
    bool destroyed;
    hlt::PlayerId player;

    SimulatedShip(int position, hlt::Halite halite, int turns_underway, hlt::PlayerId player)
      : position(position),
        halite(halite),
        turns_underway(turns_underway),
        halite_per_turn(-1),
        destroyed(false),
        player(player)
    {
    }
};

struct RandomPolicy {
    int get_move(std::mt19937& generator) {
        // distribution is inclusive
        std::uniform_int_distribution<int> distribution(1, ALL_DIRECTIONS.size()-1);
        return distribution(generator);
    }
};

struct GravityPolicy {
    GravityGrid& mining_grid;
    GravityGrid& return_grid;

    GravityPolicy(GravityGrid& mining_grid, GravityGrid& return_grid)
      : mining_grid(mining_grid),
        return_grid(return_grid)
    {
    }

    float get_move_weight(int move, int position, hlt::Halite current_halite) {
        float pull_fraction =
            ((float)hlt::constants::MAX_HALITE-current_halite)/hlt::constants::MAX_HALITE;
        float return_fraction = 1.0-pull_fraction;
        float res = 0;
        res += pull_fraction*mining_grid.get_pull(position, move);
        res += return_fraction*return_grid.get_pull(position, move);
        return res;
    }
};

// Wrapper around other policies that allows removing moves
// This is useful for avoiding collisions in rollouts.
struct MovePolicy {
    GravityPolicy policy;

    MovePolicy(GravityPolicy policy)
      : policy(policy)
    {
    }

    int get_move(std::mt19937& rng, int position, hlt::Halite current_halite, bool allowed_moves[4]) {
        float weights[4];
        for (size_t move=1; move < ALL_DIRECTIONS.size(); move++) {
            if (allowed_moves[move-1]) {
                weights[move-1] = policy.get_move_weight(move, position, current_halite);
            } else {
                weights[move-1] = 0;
            }
        }

        float sum_weights = 0;
        for (auto w : weights) { sum_weights += w; }
        if (sum_weights == 0) { return 0; }

        float rand = std::uniform_real_distribution<float>()(rng);
        float move_treshold = 0;
        for (size_t move = 1; move < ALL_DIRECTIONS.size(); move++) {
            move_treshold += weights[move-1]/sum_weights;
            if (rand < move_treshold) { return move; }
        }
        return 0;
    }
};

struct MiningPolicy {
    int map_size;

    MiningPolicy(int map_size)
      : map_size(map_size)
    {
    }

    bool should_mine(std::mt19937& generator, hlt::Halite halite_at_position, hlt::Halite total_halite) {
        float average_halite = ((float)total_halite)/map_size;
        float treshold = halite_at_position/(2*average_halite);
        float rand = std::uniform_real_distribution<float>()(generator);
        return rand < treshold;
    }
};

struct MctsSimulation {
    const Frame& frame;
    int width;
    int height;

    std::vector<SimulatedShip> original_ships;
    std::vector<hlt::Halite> original_halite;
    hlt::Halite total_halite;

    std::mt19937& generator;
    MiningPolicy mining_policy;
    std::vector<MovePolicy> move_policies;

    // The distances to each dropoff from each position for each player
    std::vector<std::vector<int>> distance_to_dropoff;

    MctsSimulation(
        std::mt19937& generator,
        const Frame& frame,
        std::vector<SimulatedShip> ships,
        std::vector<MovePolicy> move_policies
    )
      : frame(frame),
        width(frame.get_game().game_map->width),
        height(frame.get_game().game_map->height),
        original_ships(ships),
        original_halite(frame.get_board_size()),
        total_halite(0),
        generator(generator),
        mining_policy(width*height),
        move_policies(move_policies)
    {
        auto& game_map = frame.get_game().game_map;
        // Initialize halite
        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                hlt::Position pos(x, y);
                auto idx = frame.get_index(pos);
                original_halite[idx] = game_map->at(pos)->halite;
                total_halite += original_halite[idx];
            }
        }

        // Initialize distance_to_dropoff
        for (auto player : frame.get_game().players) {
            std::vector<int> distances(width*height);
            for (int y=0; y < height; y++) {
                for (int x=0; x < width; x++) {
                    int shipyard_dist = game_map->calculate_distance(
                        hlt::Position(x, y),
                        player->shipyard->position
                    );
                    distances[y*width+x] = shipyard_dist;
                }
            }
            distance_to_dropoff.push_back(distances);
        }
    }

    int move_position(int position, int move) {
        switch (move) {
            case STILL_INDEX: return position;
            case NORTH_INDEX: return pos_mod(position-width, width*height);
            case SOUTH_INDEX: return (position+width)%(width*height);
            case EAST_INDEX:
                if (position%width == width-1) {
                    return position - (width-1);
                } else {
                    return position + 1;
                }
            case WEST_INDEX:
                if (position%width == 0) {
                    return position + (width-1);
                } else {
                    return position - 1;
                }
            default: return position;
        }
    }

    int get_distance_to_dropoff(int pos, int player) {
        return distance_to_dropoff[player][pos];
    }

    std::vector<float> run(const ShipMoves& moves, int max_depth) {
        std::vector<SimulatedShip> ships(original_ships);
        // Moves taken by each ship. Stored so that all ships can be updated at the end.
        std::vector<int> taken_moves(ships.size());

        // length = board_size
        std::vector<int> num_ships_in_cell(frame.get_board_size());
        std::vector<hlt::Halite> halite(original_halite);
        // For each position, how many ships are within inspiration range
        std::vector<int> num_inspiring_ships(frame.get_board_size());
        // For each player, how many own ships within range to subtract.
        std::vector<std::vector<int>> num_own_inspiring_ships;

#ifdef DEBUG
        // All taken moves, for debug purposes
        std::vector<std::vector<int>> all_taken_moves(ships.size());
        std::vector<std::vector<int>> all_current_halite(ships.size());
        std::vector<std::vector<float>> all_current_res(ships.size());
#endif

        for (size_t i=0; i < frame.get_game().players.size(); i++) {
            num_own_inspiring_ships.push_back(std::vector<int>(frame.get_board_size()));
        }
        // Initialization
        for (auto& ship : ships) {
            num_ships_in_cell[ship.position] = 1;
            add_inspiration(num_inspiring_ships, ship.position, 1);
            add_inspiration(num_own_inspiring_ships[ship.player], ship.position, 1);
        }

        // Simulation
        for (int depth=0; depth < max_depth; depth++) {
            // Update ships and halite
            for (size_t ship_idx=0; ship_idx < ships.size(); ship_idx++) {
                auto& ship = ships[ship_idx];
                if (ship.destroyed) { continue; }

                auto inspiration_count =
                    num_inspiring_ships[ship.position]-num_own_inspiring_ships[ship.player][ship.position];
                bool is_inspired = (inspiration_count >= hlt::constants::INSPIRATION_SHIP_COUNT);

                int move;
                if (moves.is_move_specified(ship_idx, depth)) {
                    move = moves.get_move(ship_idx, depth);
                } else {
                    bool should_mine =
                        mining_policy.should_mine(generator, halite[ship.position], total_halite);
                    if (should_mine) {
                        move = STILL_INDEX;
                    } else {
                        auto& policy = move_policies[ship.player];
                        // avoid collisions
                        bool possible_moves[4];
                        for (size_t move=1; move < ALL_DIRECTIONS.size(); move++) {
                            int neighbor = move_position(ship.position, move);
                            possible_moves[move-1] = (num_ships_in_cell[neighbor] == 0);
                        }
                        move = policy.get_move(generator, ship.position, ship.halite, possible_moves);
                    }
                }
                // Not possible to move
                if (halite[ship.position]/hlt::constants::MOVE_COST_RATIO > ship.halite) {
                    move = STILL_INDEX;
                }
                taken_moves[ship_idx] = move;
                if (ALL_DIRECTIONS[move] == hlt::Direction::STILL) {
                    // Mining
                    auto max_mined = ceil_div(halite[ship.position], hlt::constants::EXTRACT_RATIO);
                    if (is_inspired) { max_mined *= hlt::constants::INSPIRED_BONUS_MULTIPLIER; }

                    auto old_halite = ship.halite;
                    ship.halite += max_mined;
                    ship.halite =
                        std::max(0, std::min(hlt::constants::MAX_HALITE, ship.halite));

                    auto removed_halite = ship.halite-old_halite;
                    if (is_inspired) { removed_halite /= hlt::constants::INSPIRED_BONUS_MULTIPLIER; }
                    halite[ship.position] -= removed_halite;
                } else {
                    // Moving
                    ship.halite -= halite[ship.position]/hlt::constants::MOVE_COST_RATIO;
                    num_ships_in_cell[ship.position]--;
                    ship.position = move_position(ship.position, move);
                    num_ships_in_cell[ship.position]++;
                }
                ship.turns_underway++;
                if (get_distance_to_dropoff(ship.position, ship.player) == 0
                    && ship.halite_per_turn < 0
                ) {
                    ship.halite_per_turn = ((float)ship.halite)/ship.turns_underway;
                }

#ifdef DEBUG
                all_taken_moves[ship_idx].push_back(move);
                all_current_halite[ship_idx].push_back(ship.halite);
                all_current_res[ship_idx].push_back(ship.halite_per_turn);
#endif
            }

            // Destroy ships
            for (auto& ship : ships) {
                if (num_ships_in_cell[ship.position] > 1) {
                    ship.destroyed = true;
                    ship.halite = 0;
                }
            }
            for (auto& ship : ships) {
                if (num_ships_in_cell[ship.position] > 1) {
                    num_ships_in_cell[ship.position] = 0;
                }
            }

            // Update inspiration
            for (size_t ship_idx = 0; ship_idx < ships.size(); ship_idx++) {
                auto& ship = ships[ship_idx];
                update_inspiration(num_inspiring_ships, ship.position, taken_moves[ship_idx]);
                update_inspiration(
                    num_own_inspiring_ships[ship.player], ship.position, taken_moves[ship_idx]);
            }
        }

        std::vector<float> res(ships.size());
        for (size_t ship_idx=0; ship_idx < ships.size(); ship_idx++) {
            auto& ship = ships[ship_idx];
            if (ship.destroyed) {
                res[ship_idx] = 0.0;
            } else if (ship.halite_per_turn >= 0) {
                res[ship_idx] = ship.halite_per_turn;
            } else {
                // An estimate of turns needed to reach a dropoff, assuming each cell has
                // an equal, non-zero amount of halite
                float turns_spent_mining =
                    ((float)hlt::constants::EXTRACT_RATIO)/hlt::constants::MOVE_COST_RATIO;
                float remaining_turns =
                    get_distance_to_dropoff(ship.position, ship.player)*(1.0+turns_spent_mining);
                res[ship_idx] = ((float)ship.halite)/(ship.turns_underway+remaining_turns);
            }
        }

#ifdef DEBUG
        std::cerr << "simulation" << std::endl;
        std::cerr << moves << std::endl;
        for (size_t ship_idx=0; ship_idx < ships.size(); ship_idx++) {
            for (int move_idx=0; move_idx < all_taken_moves[ship_idx].size(); move_idx++) {
                std::cerr << all_taken_moves[ship_idx][move_idx] << ":"
                    << all_current_halite[ship_idx][move_idx] << "="
                    << all_current_res[ship_idx][move_idx] << " ";
            }
            std::cerr << "= " << res[ship_idx] << std::endl;
        }
#endif

        return res;
    }

    static void update_inspiration(std::vector<int>& inspiration_grid, int position, int last_move) {
        // TODO
    }

    static void add_inspiration(std::vector<int>& inspiration_grid, int position, int amount) {
        // TODO
    }
};

MctsBot::MctsBot(unsigned int seed)
  : generator(seed),
    mining_grid(0, 0)
{
}

void MctsBot::init(hlt::Game& game) {
    auto& map = *game.game_map;

    mining_grid = GravityGrid(map.width, map.height);
    for (int y=0; y < map.height; y++) {
        for (int x=0; x < map.width; x++) {
            auto halite = map.at(hlt::Position(x, y))->halite;
            mining_grid.add_gravity(x, y, halite*halite);
        }
    }

    auto dropoff_pull = (10*hlt::constants::MAX_HALITE)*(10*hlt::constants::MAX_HALITE);
    return_grids = std::vector<GravityGrid>();
    for (auto player : game.players) {
        GravityGrid grid(map.width, map.height);
        auto shipyard_pos = player->shipyard->position;
        grid.add_gravity(shipyard_pos.x, shipyard_pos.y, dropoff_pull);
        // TODO dropoffs
        return_grids.push_back(grid);
    }

    game.ready("mcts");
}

void MctsBot::maintain(const hlt::Game& game) {
    auto& game_map = *game.game_map;
    // Update the existing gravity grid so no significant time is spend on it each turn.
    for (int y=0; y < game_map.height; y++) {
        for (int x=0; x < game_map.width; x++) {
            auto halite = game_map.at(hlt::Position(x, y))->halite;
            mining_grid.set_gravity(x, y, halite*halite);
        }
    }

    // Update turns_underway
    for (auto player : game.players) {
        for (auto id_ship : player->ships) {
            auto cell = game_map.at(id_ship.second->position);
            if (cell->structure && cell->structure->owner == player->id) {
                turns_underway[id_ship.first] = 0;
            } else {
                // Will insert if it is not already there.
                turns_underway[id_ship.first]++;
            }
        }
    }
}

std::vector<hlt::Command> MctsBot::run(const hlt::Game& game, time_point end_time) {
#ifdef DEBUG
    if (game.turn_number > 10) { throw "die"; }
    std::cerr << "turn: " << game.turn_number << std::endl;
#endif
    maintain(game);

    Frame frame(game);

    // Get list of all ships
    std::vector<std::shared_ptr<hlt::Ship>> all_ships;
    for (auto player : game.players) {
        for (auto id_ship : player->ships) {
            all_ships.push_back(id_ship.second);
        }
    }
    std::vector<SimulatedShip> simulation_ships;
    for (auto ship : all_ships) {
        SimulatedShip simulated(
            frame.get_index(ship->position),
            ship->halite,
            turns_underway[ship->id],
            ship->owner
        );
        simulation_ships.push_back(simulated);
    }

    // Setup simulation
    std::vector<MovePolicy> move_policies;
    for (size_t player_idx=0; player_idx < game.players.size(); player_idx++) {
        move_policies.push_back(MovePolicy(GravityPolicy(mining_grid, return_grids[player_idx])));
    }
    MctsSimulation simulation(generator, frame, simulation_ships, move_policies);

    ShipMoves simulation_moves(all_ships.size());
    // Run simulations where no moves have been specified.
    // Only used to initialize expectations for ships at dropoffs as its expectation is of lower quality.
    std::vector<float> simulation_score_sum(all_ships.size());
    for (int i=0; i < NUM_INIT_SIMULATIONS; i++) {
        auto res = simulation.run(simulation_moves, MAX_DEPTH);
        for (size_t ship_idx=0; ship_idx < res.size(); ship_idx++) {
            simulation_score_sum[ship_idx] += res[ship_idx];
        }
    }

    // Setup mcts trees
    std::vector<MctsTree> mcts_trees;
    for (size_t ship_idx=0; ship_idx < simulation_ships.size(); ship_idx++) {
        float comparison_score = 0;
        if (simulation_ships[ship_idx].turns_underway == 0) {
            comparison_score = simulation_score_sum[ship_idx]/NUM_INIT_SIMULATIONS;
        } else {
            comparison_score = last_average_scores[all_ships[ship_idx]->id];
        }
        mcts_trees.emplace_back(comparison_score);
    }

    // Run simulations and update trees
    int depth = 0;
    for (auto now = ms_clock::now(); now < end_time; now = ms_clock::now()) {
#ifdef DEBUG
        if (depth == 10) break;
#endif
        depth++;

        for (size_t ship_idx=0; ship_idx < mcts_trees.size(); ship_idx++) {
            // Sets the move in the ShipMoves buffer
            mcts_trees[ship_idx].tree_policy(simulation_moves, ship_idx);
        }

        //std::cerr << simulation_moves << std::endl;

        for (size_t possible_move = 0; possible_move < ALL_DIRECTIONS.size(); possible_move++) {
            for (size_t ship_idx=0; ship_idx < mcts_trees.size(); ship_idx++) {
                simulation_moves.push_temp(ship_idx, possible_move);
            }
            auto results = simulation.run(simulation_moves, MAX_DEPTH);
            for (size_t ship_idx=0; ship_idx < mcts_trees.size(); ship_idx++) {
                mcts_trees[ship_idx].update(simulation_moves, ship_idx, results[ship_idx]);
            }
            for (size_t ship_idx=0; ship_idx < mcts_trees.size(); ship_idx++) {
                simulation_moves.pop(ship_idx);
            }
        }
/*
        for (size_t ship_idx=0; ship_idx < mcts_trees.size(); ship_idx++) {
            std::cerr << mcts_trees[ship_idx] << std::endl;
        }
*/
    }
#ifdef DEBUG
    std::cerr << "depth: " << depth  << std::endl;
#endif

    // Update last_average_scores
    for (size_t ship_idx=0; ship_idx < all_ships.size(); ship_idx++) {
        last_average_scores[all_ships[ship_idx]->id] = mcts_trees[ship_idx].get_average_score();
    }

    std::vector<hlt::Command> commands;
    for (size_t ship_idx=0; ship_idx < all_ships.size(); ship_idx++) {
        if (all_ships[ship_idx]->owner == game.my_id) {
            commands.push_back(all_ships[ship_idx]->move(mcts_trees[ship_idx].best_move()));
        }
    }
    if (game.me->halite >= 1000) {
        commands.push_back(game.me->shipyard->spawn());
    }
    return commands;
}
