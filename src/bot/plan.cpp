#include "plan.hpp"

Plan::Plan()
  : path(SearchPath()),
    execution_step(0)
{
}

Plan::Plan(SearchPath path)
  : path(path),
    execution_step(0)
{
}

Plan::Plan(Path path)
  : execution_step(0)
{
    for (auto dir : path) {
        this->path.push_back(PathSegment(dir, -1));
    }
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
    return path[path.size()-1].halite;
}

std::ostream& operator<<(std::ostream& os, const Plan& val) {
    os << "[ ";
    for (auto segment : val.path) {
        os << segment.direction << " ";
    }
    os << "]";
    return os;
}
