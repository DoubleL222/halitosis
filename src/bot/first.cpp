#include "bot/first.hpp"
#include "bot/frame.hpp"

Plan::Plan()
  : path(Path()),
    execution_step(0)
{
}

Plan::Plan(Path path)
  : path(path),
    execution_step(0)
{
}

bool Plan::is_finished() {
    return execution_step >= path.size();
}

hlt::Direction Plan::next_move() const {
    return path[execution_step];
}

void Plan::advance() {
    execution_step++;
}

FirstBot::FirstBot(unsigned int seed)
    : rng(seed)
{
}

void FirstBot::init(hlt::Game& game) {
    // Last thing to call
    game.ready("FirstBot");
}

std::vector<hlt::Command> FirstBot::run(const hlt::Game& game) {
    Frame frame(game);

    auto player = frame.get_game().me;

    std::vector<hlt::Command> commands;
    if (player->halite >= 1000 && !game.game_map->at(player->shipyard)->is_occupied()) {
        commands.push_back(player->shipyard->spawn());
    }

    std::unordered_map<hlt::EntityId, hlt::Direction> moves;
    for (auto& pair: player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        // Create plans if necessary.
        if (plans[ship->id].is_finished()) {
            auto path = frame.get_optimal_path(*ship, player->shipyard->position);
            plans[id] = Plan(path);
        }
        moves[id] = plans[id].next_move();
    }
    auto new_moves = frame.avoid_collisions(moves);

    // Find moves for each ship, avoiding collisions.
    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        if (moves[id] == new_moves[id]) {
            plans[id].advance();
        }
        commands.push_back(ship->move(new_moves[id]));
    }
    return commands;
}
