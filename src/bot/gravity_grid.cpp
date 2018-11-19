#include "bot/gravity_grid.hpp"

std::ostream& operator<<(std::ostream& os, const OmniDirectionalValue& val) {
    os << "{u:" << val.up << ", r:" << val.right
        << ", d:" << val.down << ", l:" << val.left << "}";
    return os;
}

void update_pull(
    Frame& frame,
    int_fast8_t x,
    int_fast8_t y,
    float gravity,
    std::vector<OmniDirectionalValue>& pull
) {
    auto& map = frame.get_game().game_map;
    auto width = map->width;
    auto height = map->height;
    for (int_fast8_t updated_y=0; updated_y < height; updated_y++) {
        for (int_fast8_t updated_x=0; updated_x < width; updated_x++) {
            // No pull on itself
            if (y == updated_y && x == updated_x) { continue; }

            // Distances to the attracters from the perspective of the updated cell.
            auto dist_right_x = (x-updated_x+width)%width;
            auto dist_left_x = (updated_x-x+width)%width;
            auto dist_down_y = (y-updated_y+height)%height;
            auto dist_up_y = (updated_y-y+height)%height;

            auto dist_x = std::min(dist_left_x, dist_right_x);
            auto dist_y = std::min(dist_up_y, dist_down_y);

            auto dist_right = (dist_right_x-1+width)%width+dist_y+1;
            auto dist_left = (dist_left_x-1+width)%width+dist_y+1;
            auto dist_up = (dist_up_y-1+height)%height+dist_x+1;
            auto dist_down = (dist_down_y-1+height)%height+dist_x+1;

            pull[updated_y*width+updated_x].left += gravity/(dist_left*dist_left);
            pull[updated_y*width+updated_x].right += gravity/(dist_right*dist_right);
            pull[updated_y*width+updated_x].up += gravity/(dist_up*dist_up);
            pull[updated_y*width+updated_x].down += gravity/(dist_down*dist_down);
        }
    }
}

GravityGrid::GravityGrid(Frame& frame)
  : frame(frame)
{
    auto& map = frame.get_game().game_map;
    auto width = map->width;
    auto height = map->height;

    attractors = std::vector<float>(width*height);
    for (int_fast8_t y=0; y < height; y++) {
        for (int_fast8_t x=0; x < width; x++) {
            hlt::Position pos(x, y);
            auto halite = map->at(pos)->halite;
            attractors[y*width+x] = halite*halite;
        }
    }

    pull = std::vector<OmniDirectionalValue>(width*height);
    for (int_fast8_t y=0; y < height; y++) {
        for (int_fast8_t x=0; x < width; x++) {
            auto idx = y*width+x;
            update_pull(frame, x, y, attractors[idx], pull);
        }
    }

    return_pull = std::vector<OmniDirectionalValue>(width*height);
    auto shipyard_pos = frame.get_game().me->shipyard->position;
    auto dropoff_gravity = std::pow(hlt::constants::MAX_HALITE*RETURN_FACTOR, 2);
    update_pull(frame, shipyard_pos.x, shipyard_pos.y, dropoff_gravity, return_pull);
}

std::ostream& operator<<(std::ostream& os, const GravityGrid& grid) {
    auto width = grid.frame.get_game().game_map->width;
    auto height = grid.frame.get_game().game_map->height;
    os << "GravitationGrid" << std::endl;
    os << "Attractors: " << std::endl;
    for (int y=0; y < height; y++) {
        for (int x=0; x < width; x++) {
            os << grid.attractors[y*width+x] << " ";
        }
        os << std::endl;
    }

    os << "Pull: " << std::endl;
    for (int y=0; y < height; y++) {
        for (int x=0; x < width; x++) {
            os << grid.pull[y*width+x] << " ";
        }
        os << std::endl;
    }
    return os;
}
