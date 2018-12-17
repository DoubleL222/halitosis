#include "bot/math.hpp"
#include "bot/mcts.hpp"
#include "bot/frame.hpp"

#include <cmath>
#include <random>

// Larger values will increase exploration, smaller will increase exploitation.
const float EXPLORATION_CONSTANT = std::sqrt(2.0);

// The maximum depth that moves will be generated for
const int MAX_DEPTH = 50;

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
        std::cerr << "best_move: " << best_child << ": " << best_total_reward << "/" << best_visits << std::endl;
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
    void tree_policy_rec(ShipMoves& moves, int ship_idx) {
        if (is_expanded()) {
            int best_child = get_best_child();
            moves.push(ship_idx, best_child);
            if (!moves.is_move_specified(ship_idx, MAX_DEPTH-1)) {
                children[best_child]->tree_policy_rec(moves, ship_idx);
            }
        } else {
            // Expand
            for (size_t move = 0; move < ALL_DIRECTIONS.size(); move++) {
                children.push_back(std::make_unique<MctsTreeNode>());
            }
        }
    }

    // Sets the path in the moves struct instead of returning a newly allocated path.
    void tree_policy(ShipMoves& moves, int ship_idx) {
        moves.clear(ship_idx);
        tree_policy_rec(moves, ship_idx);
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

struct SimulatedShip {
    int position;
    hlt::Halite stored_halite;
    hlt::Halite deposited_halite;
    hlt::PlayerId player;
    bool destroyed;

    void move(int move, int width, int height) {
        switch (move) {
            case STILL_INDEX:
                break;
            case NORTH_INDEX:
                position = pos_mod(position-width, width*height);
                break;
            case SOUTH_INDEX:
                position = (position+width)%(width*height);
                break;
            case EAST_INDEX:
                if (position%width == width-1) {
                    position -= width-1;
                } else {
                    position += 1;
                }
            case WEST_INDEX:
                if (position%width == 0) {
                    position += width-1;
                } else {
                    position -= 1;
                }
                break;
        }
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

    int get_move(std::mt19937& generator, int position, hlt::Halite current_halite) {
        float pull_fraction =
            ((float)hlt::constants::MAX_HALITE-current_halite)/hlt::constants::MAX_HALITE;
        float return_fraction = 1.0-pull_fraction;

        float pulls[ALL_DIRECTIONS.size()] = {0};
        for (size_t move=1; move < ALL_DIRECTIONS.size(); move++) {
            pulls[move] += pull_fraction*mining_grid.get_pull(position, move);
            pulls[move] += return_fraction*return_grid.get_pull(position, move);
        }

        float pull_sum = 0;
        for (auto v : pulls) {
            pull_sum += v;
        }

        float rand = std::uniform_real_distribution<float>()(generator);
        float move_treshold = 0;
        for (size_t move=1; move < ALL_DIRECTIONS.size(); move++) {
            move_treshold += pulls[move]/pull_sum;
            if (rand < move_treshold) return move;
        }
        return STILL_INDEX;
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

    std::mt19937 generator;
    MiningPolicy mining_policy;
    std::vector<GravityPolicy> move_policies;
    //RandomPolicy move_policy;

    MctsSimulation(
        unsigned int seed,
        const Frame& frame,
        std::vector<std::shared_ptr<hlt::Ship>>& ships,
        std::vector<GravityPolicy> move_policies
    )
      : frame(frame),
        width(frame.get_game().game_map->width),
        height(frame.get_game().game_map->height),
        original_ships(ships.size()),
        original_halite(frame.get_board_size()),
        generator(seed),
        mining_policy(width*height),
        move_policies(move_policies)
    {
        for (size_t i=0; i<ships.size(); i++) {
            auto ship = ships[i];
            auto& orig_ship = original_ships[i];
            orig_ship.position = frame.get_index(ship->position);
            orig_ship.stored_halite = ship->halite;
            orig_ship.deposited_halite = 0;
            orig_ship.player = ship->owner;
            orig_ship.destroyed = false;
        }

        for (int y=0; y < height; y++) {
            for (int x=0; x < width; x++) {
                hlt::Position pos(x, y);
                auto idx = frame.get_index(pos);
                original_halite[idx] = frame.get_game().game_map->at(pos)->halite;
                total_halite += original_halite[idx];
            }
        }
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
                        move = policy.get_move(generator, ship.position, ship.stored_halite);
                    }
                }
                // Not possible to move
                if (halite[ship.position]/hlt::constants::MOVE_COST_RATIO > ship.stored_halite) {
                    move = STILL_INDEX;
                }
                taken_moves[ship_idx] = move;
                if (ALL_DIRECTIONS[move] == hlt::Direction::STILL) {
                    // Mining
                    auto max_mined = ceil_div(halite[ship.position], hlt::constants::EXTRACT_RATIO);
                    if (is_inspired) { max_mined *= hlt::constants::INSPIRED_BONUS_MULTIPLIER; }

                    auto old_halite = ship.stored_halite;
                    ship.stored_halite += max_mined;
                    ship.stored_halite =
                        std::max(0, std::min(hlt::constants::MAX_HALITE, ship.stored_halite));

                    auto removed_halite = ship.stored_halite-old_halite;
                    if (is_inspired) { removed_halite /= hlt::constants::INSPIRED_BONUS_MULTIPLIER; }
                    halite[ship.position] -= removed_halite;
                } else {
                    // Moving
                    ship.stored_halite -= halite[ship.position]/hlt::constants::MOVE_COST_RATIO;
                    num_ships_in_cell[ship.position]--;
                    ships[ship_idx].move(move, width, height);
                    num_ships_in_cell[ship.position]++;
                }
            }

            // Destroy ships
            for (auto& ship : ships) {
                if (num_ships_in_cell[ship.position] > 1) {
                    ship.destroyed = true;
                    ship.stored_halite = 0;
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

        std::vector<hlt::Halite> collected_halite(ships.size());
        // Set gain of each ship
        for (size_t ship_idx=0; ship_idx < ships.size(); ship_idx++) {
            auto total_halite = ships[ship_idx].stored_halite+ships[ship_idx].deposited_halite;
            collected_halite[ship_idx] = total_halite-original_ships[ship_idx].stored_halite;
        }
        hlt::Halite min_gain = 100000;
        hlt::Halite max_gain = -100000;
        for (auto v : collected_halite) {
            min_gain = std::min(min_gain, v);
            max_gain = std::max(max_gain, v);
        }
        std::vector<float> res(ships.size());
        for (size_t ship_idx = 0; ship_idx < ships.size(); ship_idx++) {
            if (min_gain == max_gain) {
                res[ship_idx] = 0.5;
            } else {
                res[ship_idx] = ((float)(collected_halite[ship_idx]-min_gain))/(max_gain-min_gain);
            }
        }
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
  : seed(seed),
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

    auto dropoff_pull = hlt::constants::MAX_HALITE*hlt::constants::MAX_HALITE;
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

std::vector<hlt::Command> MctsBot::run(const hlt::Game& game, time_point end_time) {
    Frame frame(game);
//    if (game.turn_number > 2) { throw "die"; }

    // Update the existing gravity grid so no significant time is spend on it each turn.
    for (int y=0; y < game.game_map->height; y++) {
        for (int x=0; x < game.game_map->width; x++) {
            auto halite = game.game_map->at(hlt::Position(x, y))->halite;
            mining_grid.set_gravity(x, y, halite*halite);
        }
    }

    std::vector<std::shared_ptr<hlt::Ship>> all_ships;
    for (auto player : game.players) {
        for (auto id_ship : player->ships) {
            all_ships.push_back(id_ship.second);
        }
    }

    // Run simulations and update trees
    std::vector<MctsTreeNode> mcts_trees(all_ships.size());
    std::vector<GravityPolicy> move_policies;
    for (size_t player_idx=0; player_idx < game.players.size(); player_idx++) {
        move_policies.push_back(GravityPolicy(mining_grid, return_grids[player_idx]));
    }
    MctsSimulation simulation(seed, frame, all_ships, move_policies);
    ShipMoves simulation_moves(all_ships.size());

    int depth = 0;
    for (auto now = ms_clock::now(); now < end_time; now = ms_clock::now()) {
        //if (depth == 4000) break;
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
    std::cerr << "depth: " << depth  << std::endl;

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
