#include "bot/first.hpp"
#include "bot/frame.hpp"
#include "bot/game_clone.hpp"

#include <algorithm>

hlt::Game FirstBot::advance_game(hlt::Game& game, std::vector<hlt::Command> moves)
{
	return hlt::Game();
}

FirstBotArgs::FirstBotArgs()
      : name("FirstBot"),
        max_turns(-1),
        max_search_depth(120),
        ship_build_factor(0.5),
        simulate_enemy_enabled(true),
        recalculate_paths_enabled(true),
        avoid_enemy_collisions_enabled(true),
        penalty_factor(SearchPenaltyFactor::Zero)
    {
    }

FirstBot::FirstBot(FirstBotArgs args)
  : args(args),
    should_build_ship(true)
{
}

void FirstBot::init(hlt::Game& game) {
    // Last thing to call
    game.ready(args.name);
}

// Create a new map, where destroyed ships have been removed.
template <typename T>
std::unordered_map<hlt::EntityId, T> remove_destroyed_ships(
    const std::unordered_map<hlt::EntityId, T>& map,
    const std::vector<std::shared_ptr<hlt::Ship>>& current_ships
) {
    std::unordered_map<hlt::EntityId, T> res;
    for (auto ship : current_ships) {
        res[ship->id] = map[ship->id];
    }
    return res;
}

void optimize_minings(
    const Frame& frame,
    const std::vector<std::shared_ptr<hlt::Ship>>& ships,
    std::unordered_map<hlt::EntityId, Plan>& plans
) {
    const int MINING_DEPTH = 25;

    // Initialization
    std::vector<bool> used(frame.get_board_size()*MINING_DEPTH);
    std::vector<int> corrected_indices(frame.get_board_size()*MINING_DEPTH);

    // Set which minings are used
    for (auto ship : ships) {
        auto& plan = plans[ship->id];
        auto pos = ship->position;
        for (size_t i=plan.execution_step; i < plan.path.size(); i++) {
            pos = frame.move(pos, plan.path[i].direction);
            if (plan.path[i].direction == hlt::Direction::STILL) {
                auto mining_idx = plan.path[i].mining_idx;
                if (mining_idx < MINING_DEPTH) {
                    used[frame.get_index(pos)*MINING_DEPTH+mining_idx] = true;
                }
            }
        }
    }

    // Set corrected indices
    for (unsigned int pos=0; pos < frame.get_board_size(); pos++) {
        int corrected = 0;
        for (int idx=0; idx < MINING_DEPTH; idx++) {
            corrected_indices[pos*MINING_DEPTH+idx] = corrected;
            corrected += used[pos*MINING_DEPTH+idx];
        }
    }

    for (auto ship : ships) {
        auto& plan = plans[ship->id];
        auto pos = ship->position;
        for (size_t i=plan.execution_step; i < plan.path.size(); i++) {
            pos = frame.move(pos, plan.path[i].direction);
            if (plan.path[i].direction == hlt::Direction::STILL) {
                auto mining_idx = plan.path[i].mining_idx;
                if (mining_idx < MINING_DEPTH) {
                    auto board_idx = frame.get_index(pos);
                    plan.path[i].mining_idx = corrected_indices[board_idx*MINING_DEPTH+mining_idx];
                }
            }
        }
    }
}

std::vector<hlt::Command> FirstBot::run(const hlt::Game& game, time_point end_time) {
    Frame frame(game);
    auto player = game.me;

    // Get ships
    std::vector<std::shared_ptr<hlt::Ship>> ships;
	for (auto& pair : player->ships) {
        ships.push_back(pair.second);
    }

    // Forcibly terminate the bot once a number of turns has passed if set.
    if (args.max_turns != -1 && game.turn_number > args.max_turns) {
        hlt::Halite halite_on_ships = 0;
        for (auto ship : ships) {
            halite_on_ships += ship->halite;
        }
        std::cerr << "Player " << player->id << ": " <<
            "collected=" << player->halite <<
            " on ships=" << halite_on_ships << std::endl;
        throw ("die");
    }

    // Update turns underway
    for (auto ship : ships) {
        auto cell = game.game_map->at(ship->position);
        if (cell->structure && cell->structure->owner == game.my_id) {
            current_turns_underway[ship->id] = 0;
        } else {
            current_turns_underway[ship->id]++;
        }
    }

	//Make game clone
	GameClone game_clone(frame);
    optimize_minings(frame, ships, plans);
	for (auto ship : ships) {
		game_clone.advance_game(plans[ship->id], *ship);
	}
    // Simulate some move that the opponent will make
    if (args.simulate_enemy_enabled) {
        for (auto other_player : game.players) {
            if (other_player->id != player->id) {
                for (auto ship : other_player->ships) {
                    auto simulated_target = game_clone.find_close_halite(ship.second->position);
                    auto dist =
                        game.game_map->calculate_distance(ship.second->position, simulated_target);
                    game_clone.set_occupied(simulated_target, dist);
                }
            }
        }
    }

    // Find ships for which to recalculate plans
    std::unordered_map<hlt::EntityId, int> recalculation_priority;
    for (auto& pair : player->ships) {
        auto ship = pair.second;
        int priority = 0;
        if (plans[ship->id].is_finished()) {
            priority += 100000;
        } else if (args.recalculate_paths_enabled) {
            auto expected_halite = plans[ship->id].expected_halite();
            if (expected_halite != -1) {
                priority += std::abs(expected_halite-ship->halite);
            }
            auto expected_total_halite = plans[ship->id].expected_total_halite();
            if (expected_total_halite != -1) {
                auto current_expectation = game_clone.get_expectation(plans[ship->id], *ship);
                priority += std::abs(expected_total_halite-current_expectation);
            }
        }
        recalculation_priority[ship->id] = priority;
    }
    std::sort(ships.begin(), ships.end(),
        [&recalculation_priority](std::shared_ptr<hlt::Ship>& a, std::shared_ptr<hlt::Ship>& b) {
            return recalculation_priority[a->id] > recalculation_priority[b->id];
        });

    // Calculate as many new plans as possible within time constraints
    int turns_left = hlt::constants::MAX_TURNS-frame.get_game().turn_number;
    for (size_t ship_idx = 0; ship_idx < ships.size() && ms_clock::now() < end_time; ship_idx++) {
        auto& ship = *ships[ship_idx];
        if (recalculation_priority[ship.id] == 0) { break; }

        auto max_depth = std::max(std::min(args.max_search_depth, turns_left-4), 0);
        unsigned int defensive_turns = (game.players.size() == 4 ? 150 : 0);
        game_clone.undo_advancement(plans[ship.id], ship);

        size_t turns_underway = plans[ship.id].is_finished() ? 0 : plans[ship.id].execution_step;
        //Make path on the map clone
        auto search_path = game_clone.get_optimal_path(
            ship,
            current_turns_underway[ship.id],
            args.penalty_factor,
            player->shipyard->position,
            end_time,
            max_depth,
            defensive_turns);

        // Some number to ensure that the paths are any good
        // Also ensure that a path was actually found
        if (search_path.search_depth > 80 && search_path.path.size() > 0) {
            bool fresh_plan = plans[ship.id].is_finished();
            Plan new_plan(search_path.path, search_path.final_halite);
            plans[ship.id] = new_plan;
            // Only calculate worth if its an original path
            if (fresh_plan) {
                auto worth = search_path.path[search_path.path.size()-1].halite;
                auto per_turn = ((float)worth)/(search_path.path.size()+turns_underway);
                auto expected_total = turns_left*per_turn;
                if (args.ship_build_factor*expected_total < hlt::constants::SHIP_COST) {
                    should_build_ship = false;
                }
            }
        }
        //Update clone map with current plan
        game_clone.advance_game(plans[ship.id], ship);
    }

    // Get moves from plans, and adjust
    std::unordered_map<hlt::EntityId, hlt::Direction> moves;
    for (auto ship : ships) {
        moves[ship->id] = plans[ship->id].next_move();
    }
    frame.ensure_moves_possible(moves);
    if (args.avoid_enemy_collisions_enabled) {
        frame.avoid_enemy_collisions(moves);
    }

    // Whether to attempt to build a ship.
    bool spawn_desired = false;
    if (player->halite >= 1000 && should_build_ship) {
        spawn_desired = true;
    }

    //Collision avoidance,
    auto collision_res = frame.avoid_collisions(moves, turns_left < 15, spawn_desired);

    std::vector<hlt::Command> commands;
    // Find moves for each ship, avoiding collisions.
    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        if (plans[id].next_move() == collision_res.safe_moves[id]) {
            plans[id].advance();
        }
        commands.push_back(ship->move(collision_res.safe_moves[id]));
	}
    if (collision_res.is_spawn_possible) {
        commands.push_back(player->shipyard->spawn());
    }

    update_previous_positions(game);
    return commands;
}

void FirstBot::update_previous_positions(const hlt::Game& game) {
    for (auto player : game.players) {
        for (auto pair : player->ships) {
            auto ship = pair.second;
            previous_positions.insert( { ship->id, ship->position } );
        }
    }
}
