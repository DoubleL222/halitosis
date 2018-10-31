#pragma once

#include "types.hpp"
#include "map_cell.hpp"

#include <vector>

namespace hlt {
    struct GameMap {
        int width;
        int height;
        std::vector<std::vector<MapCell>> cells;
		enum class DistanceMeasure : char {
			EUCLIDEAN = 'e',
			MANHATTAN = 'm'
		};

        MapCell* at(const Position& position) {
            Position normalized = normalize(position);
            return &cells[normalized.y][normalized.x];
        }

        MapCell* at(const Entity& entity) {
            return at(entity.position);
        }

        MapCell* at(const Entity* entity) {
            return at(entity->position);
        }

        MapCell* at(const std::shared_ptr<Entity>& entity) {
            return at(entity->position);
        }

        int calculate_distance(const Position& source, const Position& target) {
            const auto& normalized_source = normalize(source);
            const auto& normalized_target = normalize(target);

            const int dx = std::abs(normalized_source.x - normalized_target.x);
            const int dy = std::abs(normalized_source.y - normalized_target.y);

            const int toroidal_dx = std::min(dx, width - dx);
            const int toroidal_dy = std::min(dy, height - dy);

            return toroidal_dx + toroidal_dy;
        }

        Position normalize(const Position& position) {
            const int x = ((position.x % width) + width) % width;
            const int y = ((position.y % height) + height) % height;
            return { x, y };
        }

        std::vector<Direction> get_unsafe_moves(const Position& source, const Position& destination) {
            const auto& normalized_source = normalize(source);
            const auto& normalized_destination = normalize(destination);

            const int dx = std::abs(normalized_source.x - normalized_destination.x);
            const int dy = std::abs(normalized_source.y - normalized_destination.y);
            const int wrapped_dx = width - dx;
            const int wrapped_dy = height - dy;

            std::vector<Direction> possible_moves;

            if (normalized_source.x < normalized_destination.x) {
                possible_moves.push_back(dx > wrapped_dx ? Direction::WEST : Direction::EAST);
            } else if (normalized_source.x > normalized_destination.x) {
                possible_moves.push_back(dx < wrapped_dx ? Direction::WEST : Direction::EAST);
            }

            if (normalized_source.y < normalized_destination.y) {
                possible_moves.push_back(dy > wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            } else if (normalized_source.y > normalized_destination.y) {
                possible_moves.push_back(dy < wrapped_dy ? Direction::NORTH : Direction::SOUTH);
            }

            return possible_moves;
        }

        Direction naive_navigate(std::shared_ptr<Ship> ship, const Position& destination) {
            // get_unsafe_moves normalizes for us
            for (auto direction : get_unsafe_moves(ship->position, destination)) {
                Position target_pos = ship->position.directional_offset(direction);
                if (!at(target_pos)->is_occupied()) {
                    at(target_pos)->mark_unsafe(ship);
                    return direction;
                }
            }

            return Direction::STILL;
        }

		std::vector<MapCell*> get_cells_within_radius(const Position& center, const int radius, const DistanceMeasure distanceMeasure) {
			int rr = radius * radius, dx, dy;
			std::vector<MapCell*> cells;
			cells.reserve(rr);

			for (int xx = center.x - radius; xx <= center.x + radius; ++xx)
				for (int yy = center.y - radius; yy <= center.y + radius; ++yy)
				{
					dx = center.x - xx;
					dy = center.y - yy;

					switch (distanceMeasure) {
						case DistanceMeasure::EUCLIDEAN:
							if ((dx * dx + dy * dy) < rr)
								cells.push_back(at(Position(xx, yy)));
							break;
						case DistanceMeasure::MANHATTAN:
							if (std::abs(dx) + std::abs(dy) < radius)
								cells.push_back(at(Position(xx, yy)));
							break;
						default:
							log::log(std::string("Error: get_positions_within_radius: unknown distance measure ") + static_cast<char>(distanceMeasure));
							exit(1);
					}
				}
			return cells;
		}

        void _update();
        static std::unique_ptr<GameMap> _generate();
    };
}
