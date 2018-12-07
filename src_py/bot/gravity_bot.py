from hlt.positionals import Position
from hlt.positionals import Direction

class GravityBot:
    def __init__(self, game, id):
        self.game = game
        self.id = id

    def move(self, position, dx, dy):
        # Get new position based on current position and move direction
        position.x += dx
        position.y += dy
        return self.game_copy.game_map.normalize(position)

    def make_vector_map(self, game_map):
        for y in range(game_map.height):
            for x in range(game_map.width):
                current_cell_pos = Position(x,y)
                current_cell = game_map[current_cell_pos]
                probability_map = {}
                probability_map[Direction.East] = 0
                probability_map[Direction.West] = 0
                probability_map[Direction.North] = 0
                probability_map[Direction.South] = 0
                for y1 in range(game_map.height):
                    for x1 in range(game_map.width):
                        comparison_cell_pos = Position(x1, y1)
                        comparison_cell = game_map[comparison_cell_pos]
                        dist_to_cell = game_map.calculate_distance(current_cell_pos, comparison_cell_pos)


    def move(self, game_map, position, direction):

        game_map.normalize()

#    def run(self, game):
