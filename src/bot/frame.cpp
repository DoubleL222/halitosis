#include "bot/frame.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <limits>

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


struct AStarNode {
	float f_val; //F is the total cost of the node. f = g + h
	float h_val; //G is the distance between the current node and the start node.
	float g_val; //H is the heuristic estimated distance from the current node to the end node.

	float movement_penalty; //weight

	AStarNode * parent = nullptr;
	hlt::Position node_position;

	AStarNode(hlt::Position pos) : node_position(pos)
	{};
};

Path Frame::get_a_star_path(hlt::GameMap & map, hlt::Position from, hlt::Position end)
{
	// Put node_start in the OPEN list 
	AStarNode* current_node = new AStarNode(from);
	current_node->h_val = map.calculate_distance(from, end);
	current_node->g_val = 0;
	current_node->movement_penalty = 0;
	AStarNode* end_node = new AStarNode(end);

	std::vector<AStarNode> open_list;
	std::vector<AStarNode> closed_list;
	open_list.push_back(*current_node);
	//while the OPEN list is not empty
	while (open_list.size() > 0) 
	{
		//Take from the open list the node node_current with the lowest f(node_current) = g(node_current) + h(node_current)
		float lowest_f = std::numeric_limits<float>::max();
		AStarNode * lowest_node = nullptr;
		for (AStarNode node : open_list) 
		{
			float current_f = node.g_val + node.h_val;
			if (current_f < lowest_f) 
			{
				lowest_f = current_f;
				lowest_node = &node;
			}
		}
		current_node = lowest_node;

		if (current_node->node_position == end_node->node_position) 
		{
			//TODO return path
			break;
		}

		//Generate each state node_successor that come after node_current
		std::array<hlt::Position, 4U> neighbours = current_node->node_position.get_surrounding_cardinals();
		//for each node_successor of node_current
		for (hlt::Position neighbour_position : neighbours) 
		{
			//Set successor_current_cost = g(node_current) + w(node_current, node_successor)
			float neighbour_current_cost = current_node->g_val + map.calculate_distance(current_node->node_position, neighbour_position);
			
			//check if in open or closed list
			bool is_in_open_list = false;
			bool is_in_closed_list = false;
			int open_list_id = -1;
			int closed_list_id = -1;
			if (open_list.size() > 0) 
			{
				for (int i = 0; i < open_list.size(); i++)
				{
					if (open_list[i].node_position == neighbour_position)
					{
						is_in_open_list = true;
						open_list_id = i;
					}
				}
			}
			if (!is_in_open_list && closed_list.size() > 0) 
			{
				for (int i = 0; i < closed_list.size(); i++)
				{
					if (closed_list[i].node_position == neighbour_position)
					{
						is_in_closed_list = true;
						closed_list_id = i;
					}
				}
			}

			AStarNode * selected_neighbour_node = nullptr;
			//if node_successor is in the OPEN list, if g(node_successor) ≤ successor_current_cost continue
			if (is_in_open_list) 
			{
				if (open_list[open_list_id].g_val <= neighbour_current_cost) 
				{
					continue;
				}
				selected_neighbour_node = &open_list[open_list_id];
			}
			//else if node_successor is in the CLOSED list, if g(node_successor) ≤ successor_current_cost continue
			else if (is_in_closed_list) 
			{
				if (closed_list[closed_list_id].g_val <= neighbour_current_cost)
				{
					continue;
				}
				//Move node_successor from the CLOSED list to the OPEN list
				open_list.push_back(closed_list[closed_list_id]);
				selected_neighbour_node = &open_list[open_list.size() - 1];
				closed_list.erase(closed_list.begin()+closed_list_id);
			}
			//else Add node_successor to the OPEN list, Set h(node_successor) to be the heuristic distance to node_goal
			else
			{
				selected_neighbour_node = new AStarNode(neighbour_position);
				selected_neighbour_node->h_val = map.calculate_distance(neighbour_position, end);
				selected_neighbour_node->g_val = 0;
				selected_neighbour_node->movement_penalty = 0;
				open_list.push_back(*selected_neighbour_node);
			}
			//Set g(node_successor) = successor_current_cost, Set the parent of node_successor to node_current
			selected_neighbour_node->g_val = neighbour_current_cost;
			selected_neighbour_node->parent = current_node;
		}
		closed_list.push_back(*current_node);

		//TODO remove node from open
		//if(open)
	}

	return Path();
}

std::unordered_map<hlt::EntityId, hlt::Direction> Frame::avoid_collisions(
    std::unordered_map<hlt::EntityId, hlt::Direction>& moves
) {
    std::unordered_map<hlt::EntityId, hlt::Direction> new_moves;
    std::unordered_map<hlt::Position, hlt::EntityId> ship_going_to_position;

    auto player = get_game().me;
    for (auto& pair : player->ships) {
        auto id = pair.first;
        auto& ship = pair.second;

        avoid_collisions_rec(new_moves, ship_going_to_position, id, ship->position, moves[id]);
    }
    return new_moves;
}

void Frame::avoid_collisions_rec(
    std::unordered_map<hlt::EntityId, hlt::Direction>& current_moves,
    std::unordered_map<hlt::Position, hlt::EntityId>& ship_going_to_position,
    hlt::EntityId ship,
    hlt::Position ship_position,
    hlt::Direction desired_move
) {
    auto new_position = move(ship_position, desired_move);
    // If another ship already would like to go to the same spot
    if (ship_going_to_position.count(new_position) == 1) {
        auto other_ship = ship_going_to_position[new_position];
        auto other_move = current_moves[other_ship];
        auto other_origin = move(new_position, hlt::invert_direction(other_move));

        bool update_other_ship = false;
        if (desired_move == hlt::Direction::STILL) { update_other_ship = true; }
        if (!update_other_ship) {
            auto own_sea_halite = get_game().game_map->at(ship_position)->halite;
            auto other_sea_halite = get_game().game_map->at(other_origin)->halite;
            update_other_ship = (own_sea_halite < other_sea_halite);
        }
        auto updated_ship = ship;
        auto updated_position = ship_position;
        if (update_other_ship) {
            current_moves[ship] = desired_move;
            ship_going_to_position[new_position] = ship;
            updated_ship = other_ship;
            updated_position = other_origin;
        }
        avoid_collisions_rec(
            current_moves,
            ship_going_to_position,
            updated_ship,
            updated_position,
            hlt::Direction::STILL);
    } else {
        current_moves[ship] = desired_move;
        ship_going_to_position[new_position] = ship;
    }
}



