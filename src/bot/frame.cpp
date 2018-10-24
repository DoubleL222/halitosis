#include "bot/frame.hpp"

Frame::Frame(const hlt::Game& game) : game(game) {}

const hlt::Game& Frame::get_game() const {
    return game;
}
