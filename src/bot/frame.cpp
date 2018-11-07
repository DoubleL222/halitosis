﻿#include "bot/frame.hpp"
#include "bot/game_clone.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <limits>
#include <queue>

Frame::Frame(const hlt::Game& game) : game(game) {}

const hlt::Game& Frame::get_game() const {
    return game;
}

hlt::Position Frame::move(hlt::Position pos, int dx, int dy) {
    pos.x += dx;
    pos.y += dy;
    return game.game_map->normalize(pos);
}

hlt::Position Frame::move(hlt::Position pos, hlt::Direction direction) {
    switch (direction) {
        case hlt::Direction::NORTH: return move(pos, 0, -1);
        case hlt::Direction::EAST: return move(pos, 1, 0);
        case hlt::Direction::SOUTH: return move(pos, 0, 1);
        case hlt::Direction::WEST: return move(pos, -1, 0);
        default: return pos;
    }
}

int get_depth_index(int depth, hlt::GameMap& map, hlt::Position pos) {
    int board_size = map.width*map.height;
    return depth*board_size+pos.y*map.width+pos.x;
}

Path Frame::get_optimal_path(hlt::Ship& ship, hlt::Position end) {
    return get_optimal_path(*game.game_map, ship, end);
}

struct SearchState {
    bool visited;
    hlt::Halite halite;
    std::unordered_map<hlt::Position, hlt::Halite> board_override;
    hlt::Direction in_direction;
};

Path get_search_path(
    hlt::GameMap& map,
    SearchState* search_state,
    hlt::Position start,
    hlt::Position end,
    int max_depth
) {
    Path res(max_depth);
    hlt::Position current_pos = end;
    for (int depth=max_depth; depth>0; depth--) {
        int idx = get_depth_index(depth, map, current_pos);
        res[depth-1] = search_state[idx].in_direction;
        current_pos = current_pos.directional_offset(hlt::invert_direction(res[depth-1]));
        current_pos = map.normalize(current_pos);
    }
    return res;
}

// Find an optimal path to a point for a ship on a specific map.
Path Frame::get_optimal_path(hlt::GameMap& map, hlt::Ship& ship, hlt::Position end) {
    auto start = ship.position;
    int board_size = map.width*map.height;
    int max_depth = std::max(map.width, map.height);
    auto search_state_owned = std::make_unique<SearchState[]>(board_size*max_depth);
    auto search_state = search_state_owned.get();

    int start_idx = get_depth_index(0, map, start);
    search_state[start_idx].halite = ship.halite;
    search_state[start_idx].visited = true;
    search_state[start_idx].board_override = std::unordered_map<hlt::Position, hlt::Halite>();
    for (int depth=0; depth < max_depth-1; depth++) {
        for (int dy=-depth; dy <= depth; dy++) {
            for (int dx=-depth; dx <= depth; dx++) {
                auto pos = move(start, dx, dy);
                int cur_idx = get_depth_index(depth, map, pos);
                if (!search_state[cur_idx].visited) { continue; }

                auto current_halite = search_state[cur_idx].halite;

                int sea_halite = search_state[cur_idx].board_override.count(pos)
                    ? search_state[cur_idx].board_override[pos]
                    : map.at(pos)->halite;
                auto halite_after_move = current_halite-std::ceil(sea_halite*0.1);
                auto halite_after_gather = current_halite;
                if (!map.at(pos)->has_structure()) {
                    halite_after_gather += (hlt::Halite)std::ceil(sea_halite*0.25);
                    halite_after_gather = std::min(halite_after_gather, hlt::constants::MAX_HALITE);
                }

                // Movement
                if (halite_after_move >= 0) {
                    for (auto direction : hlt::ALL_CARDINALS) {
                        int new_idx = get_depth_index(depth+1, map, move(pos, direction));
                        bool update = !search_state[new_idx].visited
                            || halite_after_move > search_state[new_idx].halite;
                        if (update) {
                            search_state[new_idx].halite = halite_after_move;
                            search_state[new_idx].board_override
                                = search_state[cur_idx].board_override;
                            search_state[new_idx].in_direction = direction;
                            search_state[new_idx].visited = true;
                        }
                    }
                }
                // Gathering
                int new_idx = get_depth_index(depth+1, map, pos);
                if (halite_after_gather > search_state[new_idx].halite) {
                    std::unordered_map<hlt::Position, hlt::Halite> new_override(
                        search_state[cur_idx].board_override
                    );
                    new_override[pos] =
                        sea_halite-(halite_after_gather-current_halite);

                    search_state[new_idx].halite = halite_after_gather;
                    search_state[new_idx].board_override = new_override;
                    search_state[new_idx].in_direction = hlt::Direction::STILL;
                    search_state[new_idx].visited = true;
                }
            }
        }
    }
    /*
    for (int depth=0; depth<max_depth; depth++) {
        std::cerr << depth << "-------" << std::endl;
        for (int dy=-depth; dy <= depth; dy++) {
            for (int dx=-depth; dx <= depth; dx++) {
                auto pos = move(start, dx, dy);
                int cur_idx = get_depth_index(depth, map, pos);
                if (search_state[cur_idx].visited) {
                    std::cerr << search_state[cur_idx].halite << " ";
                } else {
                    std::cerr << "X ";

                }
            }
            std::cerr << std::endl;
        }
    }
    */
   
    float best_halite_per_turn = 0;
    int best_depth = 1;
    for (int depth=1; depth < max_depth; depth++) {
        int idx = get_depth_index(depth, map, end);
        float halite_per_turn = ((float)(search_state[idx].halite))/depth;
        if (halite_per_turn > best_halite_per_turn) {
            best_halite_per_turn = halite_per_turn;
            best_depth = depth;
        }
    }
    return get_search_path(map, search_state, start, end, best_depth);
}

int Frame::get_index(hlt::Position position) const {
    return position.y*game.game_map->width+position.x;
}

Path Frame::get_direct_path(hlt::GameMap & map, hlt::Ship & ship, hlt::Position end) {
	Path unsafe_path = map.get_unsafe_moves(ship.position, end);
	return unsafe_path;
}

int Frame::get_optimal_number_of_ships(float factor_halite_left, float factor_turns_left)
{
	return (int)(factor_halite_left * (float)get_halite_left() + factor_turns_left * (float)get_turns_left());
}

int Frame::get_helite_left_on_map(hlt::GameMap & map)
{
	int halite_left = 0.0f;
	for (std::vector<hlt::MapCell> cell_row : map.cells) 
	{
		for (hlt::MapCell cell : cell_row) 
		{
			halite_left += cell.halite;
		}
	}
	return halite_left;
}

int Frame::get_halite_left()
{
	return get_helite_left_on_map(*game.game_map);
}

int Frame::get_turns_left()
{
	return hlt::constants::MAX_TURNS - game.turn_number;
}

unsigned int Frame::get_board_size() const {
    return game.game_map->width*game.game_map->height;
}

struct Edge {
    unsigned int from;
    unsigned int to;
    bool full;
    bool priority;

    Edge(unsigned int from, unsigned int to, bool priority)
      : from(from),
        to(to),
        full(false),
        priority(priority)
    {
    }

    // The neighbor to a specific node when traversing this edge.
    int neighbor(unsigned int current_node) const {
        return current_node == to ? from : to;
    }

    // Whether the edge can be traversed from the specified node.
    bool is_traversable(unsigned int current_node, bool priority_only) const {
        return (priority || !priority_only) && (current_node == from ? !full : full);
    }

    // Send 1 unit of flow through this edge.
    void flow() {
        full = !full;
    }
};

std::ostream& operator<<(std::ostream& os, const Edge& self) {
    os << "Edge { from: " << self.from << ", to: " << self.to << ", full: " << self.full << ", priority: " << self.priority << " }";
    return os;
}

struct FlowGraph {
    std::vector<Edge> edges;
    std::vector<std::vector<int>> nodes;
    std::vector<int> last_visit;
    std::vector<int> from_edge;
    int current_depth = 0;

    void finalize(int size) {
        nodes = std::vector<std::vector<int>>(size);
        last_visit = std::vector<int>(size);
        from_edge = std::vector<int>(size);

        for (int i=0; i<size; i++) {
            last_visit[i] = -1;
        }
        for (size_t i=0; i<edges.size(); i++) {
            nodes[edges[i].from].push_back(i);
            nodes[edges[i].to].push_back(i);
        }
    }

    bool augment(bool priority_only) {
        current_depth++;

        std::queue<int> queue;
        queue.push(0);
        last_visit[0] = current_depth;
        while (!queue.empty()) {
            int front = queue.front();
            queue.pop();
            // Stop if destination is reached.
            if (front == 1) { break; }

            for (auto edge_idx : nodes[front]) {
                if (edges[edge_idx].is_traversable(front, priority_only)) {
                    int neighbor = edges[edge_idx].neighbor(front);
                    if (last_visit[neighbor] != current_depth) {
                        last_visit[neighbor] = current_depth;
                        from_edge[neighbor] = edge_idx;
                        queue.push(neighbor);
                    }
                }
            }
        }
        if (last_visit[1] != current_depth) {
            // No augmentation path found
            return false;
        }
        int back_node = 1;
        while (back_node != 0) {
            auto& edge = edges[from_edge[back_node]];
            edge.flow();
            back_node = edge.neighbor(back_node);
        }
        return true;
    }
};

std::ostream& operator<<(std::ostream& os, const FlowGraph& graph) {
    for (size_t i=0; i<graph.nodes.size(); i++) {
        int relevant_neighbors = 0;
        for (auto edge_idx: graph.nodes[i]) {
            int neighbor = graph.edges[edge_idx].neighbor(i);
            if (graph.nodes[neighbor].size() >= 2) {
                relevant_neighbors++;
            }
        }

        if (relevant_neighbors >= 2 || i == 0) {
            os << i << ": ";
            for (auto edge_idx: graph.nodes[i]) {
                int neighbor = graph.edges[edge_idx].neighbor(i);
                if (graph.nodes[neighbor].size() >= 2) {
                    os << graph.edges[edge_idx] << " ";
                }
            }
            os << std::endl;
        }
    }
    return os;
}

std::unordered_map<hlt::EntityId, hlt::Direction> Frame::avoid_collisions(
    std::unordered_map<hlt::EntityId, hlt::Direction>& moves
) {
    // Reduce to flow problem.
    // There are nodes for each ship, each cell + source, sink.
    // The source is connected to each ship with capacity 1.
    // Each ship is connected to each cell it can go to with capacity 1.
    // Each cell is connected to the sink with capacity 1.
    // The returned moves are those where a path between the ships and cells have been used.
    //
    // To get as many prefered paths as possible, first find max flow using only desired moves,
    // then augment the solution using bfs with all edges.
    FlowGraph graph;
    std::vector<std::shared_ptr<hlt::Ship>> ships;
    for (auto pair : game.me->ships) {
        ships.push_back(pair.second);
    }

    // Setup graph
    // source = 0, sink = 1
    for (size_t i=0; i<ships.size(); i++) {
        graph.edges.push_back(Edge(0, 2+i, true));
        int cell_index = get_index(ships[i]->position);
        int next_index = get_index(move(ships[i]->position, moves[ships[i]->id]));
        graph.edges.push_back(Edge(2+i, 2+ships.size()+next_index, true));
        if (next_index != cell_index) {
            graph.edges.push_back(Edge(2+i, 2+ships.size()+cell_index, false));
        }
    }
    for (size_t i=0; i<get_board_size(); i++) {
        graph.edges.push_back(Edge(2+ships.size()+i, 1, true));
    }
    graph.finalize(2+ships.size()+get_board_size());

    // Solve flow using priority edges only.
    while(graph.augment(true) != 0) { }
    // Augment using all edges
    while(graph.augment(false) != 0) { }

    std::unordered_map<hlt::EntityId, hlt::Direction> res;
    for (size_t i=0; i<ships.size(); i++) {
        int cell_index = get_index(ships[i]->position);

        hlt::Direction new_move = hlt::Direction::STILL;
        for (auto& edge_idx : graph.nodes[2+i]) {
            auto edge = graph.edges[edge_idx];
            if (edge.from == 2+i && edge.full) {
                if (edge.to == 2+ships.size()+cell_index) {
                    new_move = hlt::Direction::STILL;
                } else {
                    new_move = moves[ships[i]->id];
                }
            }
        }
        res[ships[i]->id] = new_move;
    }
    return res;
}

std::vector<hlt::MapCell*> Frame::get_cells_within_radius(const hlt::Position& center, const int radius, const Frame::DistanceMeasure distanceMeasure) {
	int rr = radius * radius, dx, dy;
	std::vector<hlt::MapCell*> cells;
	cells.reserve(rr);

	for (int xx = center.x - radius; xx <= center.x + radius; ++xx)
		for (int yy = center.y - radius; yy <= center.y + radius; ++yy)
		{
			dx = center.x - xx;
			dy = center.y - yy;

			switch (distanceMeasure) {
			case Frame::DistanceMeasure::EUCLIDEAN:
				if ((dx * dx + dy * dy) < rr)
					cells.push_back(game.game_map->at(hlt::Position(xx, yy)));
				break;
			case Frame::DistanceMeasure::MANHATTAN:
				if (std::abs(dx) + std::abs(dy) < radius)
					cells.push_back(game.game_map->at(hlt::Position(xx, yy)));
				break;
			default:
				hlt::log::log(std::string("Error: get_positions_within_radius: unknown distance measure ") + static_cast<char>(distanceMeasure));
				exit(1);
			}
		}
	return cells;
}

int Frame::get_total_halite_in_cells_within_radius(const hlt::Position& center, const int radius, const Frame::DistanceMeasure distanceMeasure) {
	std::vector<hlt::MapCell*> cells = get_cells_within_radius(center, radius, distanceMeasure);
	int total = 0;
	for (hlt::MapCell* cell : cells) {
		total += cell->halite;
	}
	return total;
}
