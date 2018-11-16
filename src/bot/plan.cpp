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

hlt::Direction Plan::next_move() const {
    if (is_finished()) {
        return hlt::Direction::STILL;
    }
    return path[execution_step].direction;
}

void Plan::advance() {
    execution_step++;
}

