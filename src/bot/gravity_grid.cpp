#include "bot/gravity_grid.hpp"
#include "bot/math.hpp"

std::ostream& operator<<(std::ostream& os, const OmniDirectionalValue& val) {
    os << "{u:" << val.up << ", r:" << val.right
        << ", d:" << val.down << ", l:" << val.left << "}";
    return os;
}

GravityGrid::GravityGrid(int width, int height)
  : width(width),
    height(height),
    attractors(width*height),
    pull(width*height)
{
}

void GravityGrid::add_gravity(int x, int y, float gravity) {
    int position = y*width+x;
    attractors[position] += gravity;

    for (int_fast8_t updated_y=0; updated_y < height; updated_y++) {
        for (int_fast8_t updated_x=0; updated_x < width; updated_x++) {
            // No pull on itself
            if (y == updated_y && x == updated_x) { continue; }

            // Distances to the attracters from the perspective of the updated cell.
            auto dist_right_x = pos_mod(x-updated_x, width);
            auto dist_left_x = pos_mod(updated_x-x, width);
            auto dist_down_y = pos_mod(y-updated_y, height);
            auto dist_up_y = pos_mod(updated_y-y, height);

            auto dist_x = std::min(dist_left_x, dist_right_x);
            auto dist_y = std::min(dist_up_y, dist_down_y);

            auto dist_right = pos_mod(dist_right_x-1, width)+dist_y+1;
            auto dist_left = pos_mod(dist_left_x-1, width)+dist_y+1;
            auto dist_up = pos_mod(dist_up_y-1, height)+dist_x+1;
            auto dist_down = pos_mod(dist_down_y-1, height)+dist_x+1;

            // would usually be /2*pi*radius, but the circumference is smaller in this case
            pull[updated_y*width+updated_x].left += gravity/(4*dist_left);
            pull[updated_y*width+updated_x].right += gravity/(4*dist_right);
            pull[updated_y*width+updated_x].up += gravity/(4*dist_up);
            pull[updated_y*width+updated_x].down += gravity/(4*dist_down);
        }
    }
}

void GravityGrid::set_gravity(int x, int y, float gravity) {
    int position = y*width+x;
    if (gravity != attractors[position]) {
        add_gravity(x, y, -attractors[position]);
        add_gravity(x, y, gravity);
    }
}

float GravityGrid::get_pull(int position, size_t move) const {
    switch (move) {
        case NORTH_INDEX: return pull[position].up;
        case SOUTH_INDEX: return pull[position].down;
        case EAST_INDEX: return pull[position].right;
        case WEST_INDEX: return pull[position].left;
        default: return 0;
    }
}

std::ostream& operator<<(std::ostream& os, const GravityGrid& grid) {
    os << "GravitationGrid" << std::endl;
    os << "Attractors: " << std::endl;
    for (int y=0; y < grid.height; y++) {
        for (int x=0; x < grid.width; x++) {
            os << grid.attractors[y*grid.width+x] << " ";
        }
        os << std::endl;
    }

    os << "Pull: " << std::endl;
    for (int y=0; y < grid.height; y++) {
        for (int x=0; x < grid.width; x++) {
            os << grid.pull[y*grid.width+x] << " ";
        }
        os << std::endl;
    }
    return os;
}
