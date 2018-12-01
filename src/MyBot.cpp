#include "bot/first.hpp"
#include "hlt/game.hpp"
#include "hlt/log.hpp"

#include <chrono>
#include <ctime>
#include <string>

int main(int argc, char* argv[]) {
	//std::cout << "Paramater: " << argv[0] << std::endl;
	unsigned int rng_seed = argc > 1
        ? static_cast<unsigned int>(std::stoul(argv[1]))
        : std::time(nullptr);

    hlt::Game game;

    FirstBotArgs args;
    args.max_turns = 100;
    args.avoid_enemy_collisions_enabled = (game.players.size() == 4);

    auto bot = FirstBot(args);
    bot.init(game);

    while (true) {
        game.update_frame();
        auto now = ms_clock::now();
        auto turn_end = now+std::chrono::milliseconds(1000);
        auto commands = bot.run(game, turn_end);
        if (!game.end_turn(commands)) {
            break;
        }
    }

    return 0;
}
