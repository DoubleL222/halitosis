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


#    def run(self, game):
