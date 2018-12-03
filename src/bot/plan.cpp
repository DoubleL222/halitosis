#include "plan.hpp"

Plan::Plan()
  : path(SearchPath()),
    execution_step(0),
    final_halite(0)
{
}

Plan::Plan(SearchPath path, hlt::Halite final_halite)
  : path(path),
    execution_step(0),
    final_halite(final_halite)
{
}

bool Plan::is_finished() const {
    return execution_step >= path.size();
}

hlt::Halite Plan::expected_halite() const {
    return path[execution_step].halite;
}

hlt::Direction Plan::next_move() const {
    if (is_finished()) {
        return hlt::Direction::STILL;
    }
    return path[execution_step].direction;
}

void Plan::advance() {
    execution_step++;
}

hlt::Halite Plan::expected_total_halite() const {
    return final_halite;
}

std::ostream& operator<<(std::ostream& os, const Plan& val) {
    os << "[ ";
    for (auto segment : val.path) {
        os << segment.direction << " ";
    }
    os << "]";
    return os;
}
