#include "bot/gravitybot.hpp"
#include "bot/frame.hpp"
#include "bot/game_clone.hpp"
#include "bot/gravity_grid.hpp"

GravityBot::GravityBot(unsigned int seed)
  : rng(seed)
{
}

void GravityBot::init(hlt::Game& game) {
    // Last thing to call
    game.ready("GravityBot");
}

std::vector<hlt::Command> GravityBot::run(const hlt::Game& game, time_point end_time) {
    Frame frame(game);
    GravityGrid scent_grid(frame);
    auto player = frame.get_game().me;

    std::vector<hlt::Command> commands;

    bool spawn_desired = false;
    if (player->halite >= 1000 && game.turn_number < 200) {
        spawn_desired = true;
    }

    std::unordered_map<hlt::EntityId, hlt::Direction> moves;

    unsigned int turns_left = hlt::constants::MAX_TURNS-frame.get_game().turn_number;
    std::unordered_map<hlt::EntityId, hlt::Halite> current_halite;
    for (auto& pair : player->ships) {
        auto ship = pair.second;
        current_halite[ship->id] = ship->halite;
    }

    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        // Create plans if necessary.
        if (plans[ship->id].is_finished()) {
            std::vector<hlt::Direction> moves;
            auto current_pos = player->shipyard->position;
            for (int i=0; i<50; i++) {
                auto move = scent_grid.get_move(current_pos, current_halite[ship->id], rng);
                current_pos = frame.move(current_pos, move);
                moves.push_back(move);
                if (move == hlt::Direction::STILL) {
                    auto mined_halite =
                        game.game_map->at(current_pos)->halite/hlt::constants::EXTRACT_RATIO;
                    current_halite[ship->id] =
                        std::min(hlt::constants::MAX_HALITE, current_halite[ship->id]+mined_halite);
                }
                auto structure = game.game_map->at(current_pos)->structure;
                if (structure && structure->owner == game.my_id) {
                    current_halite[ship->id] = 0;
                }
            }
            plans[id] = Plan(moves);
        }
        moves[id] = plans[id].next_move();
    }

    frame.ensure_moves_possible(moves);
    //Collision avoidance,
    auto collision_res = frame.avoid_collisions(moves, turns_left < 15, spawn_desired);

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
    return commands;
}
