#include "bot/first.hpp"
#include "hlt/game.hpp"
#include "hlt/log.hpp"

#include <ctime>
#include <string>

int main(int argc, char* argv[]) {
    unsigned int rng_seed = argc > 1
        ? static_cast<unsigned int>(std::stoul(argv[1]))
        : std::time(nullptr);

    hlt::Game game;

    auto bot = FirstBot(rng_seed);
    bot.init(game);

    hlt::log::log("Successfully created bot! My Player ID is " + std::to_string(game.my_id) + ". Bot rng seed is " + std::to_string(rng_seed) + ".");

    for (;;) {
        game.update_frame();
        auto commands = bot.run(game);
        if (!game.end_turn(commands)) {
            break;
        }
    }

    return 0;
}
