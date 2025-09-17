#include <cstdint>
#include <array>
#include <string>
#include <memory>

#include "raylib.h"
#include "movement_dir.h"
#include "entity.h"
#include "tile_map.h"
#include "player.h"
#include "ghosts.h"

struct Entities {
  Entity player;
  Entity blinky;
  Entity pinky;
  Entity inky;
  Entity clyde;
};

template<std::uint16_t Rows>
static std::pair<std::unique_ptr<TileMap>, std::unique_ptr<Entities>>
parse_level(const std::array<std::string, Rows>& level, std::uint16_t tile_size);

static void check_and_resolve_entity_collisions(Entities* entities);

static void draw_map_and_entities(const TileMap& tile_map, Entities* entities,
                                  GHOST_STATE curr_ghost_state, float dt);

static void draw_end_game_text(const char* msg,
                               std::uint32_t screen_width,
                               std::uint32_t screen_height);
int main(void) {
  // The tile map variables, tile map itself and scatter/chase schedules
  // are all placed here for convenience. Ultimately, they can be taken out
  // into a level/map file
  const std::uint16_t tile_size = 24;
  const std::uint16_t num_tiles_x = 28;
  const std::uint16_t num_tiles_y = 36;
  const std::uint32_t screen_width = num_tiles_x * tile_size;
  const std::uint32_t screen_height = num_tiles_y * tile_size;

  std::array<std::string, num_tiles_y> level = {
    "                            ",
	"                            ",
	"                            ",
	"############################",
	"#............##............#",
	"#.####.#####.##.#####.####.#",
	"#O####.#####.##.#####.####O#",
	"#.####.#####.##.#####.####.#",
	"#..........................#",
	"#.####.##.########.##.####.#",
	"#.####.##.########.##.####.#",
	"#......##....##....##......#",
	"######.##### ## #####.######",
	"     #.##### ## #####.#     ",
	"     #.##    B     ##.#     ",
	"     #.## ###--### ##.#     ",
	"######.## #      # ##.######",
	"=     .   #I K C #   .     =",
	"######.## #      # ##.######",
	"     #.## ######## ##.#     ",
	"     #.##          ##.#     ",
	"     #.## ######## ##.#     ",
	"######.## ######## ##.######",
	"#............##............#",
	"#.####.#####.##.#####.####.#",
	"#.####.#####.##.#####.####.#",
	"#O..##.......P .......##..O#",
	"###.##.##.########.##.##.###",
	"###.##.##.########.##.##.###",
	"#......##....##....##......#",
	"#.##########.##.##########.#",
	"#.##########.##.##########.#",
	"#..........................#",
	"############################",
	"                            ",
	"                            "
  };

  // Ghosts time schedule for scattering and chasing, in seconds
  constexpr double scatter_schedule[4] = {7.0, 7.0, 5.0, 5.0};
  constexpr double chase_schedule[4]   = {20.0, 20.0, 20.0,
                                          std::numeric_limits<double>::infinity()};

  // Init
  InitWindow(screen_width, screen_height, "Pacman");
  SetTargetFPS(60);

  auto [tile_map_ptr, entities_ptr] = parse_level(level, tile_size);

  // Using these locals to avoid dereferencing syntax
  TileMap& tile_map = *tile_map_ptr;
  Entities& entities = *entities_ptr;

  GhostsStateMachine ghosts_phase = {};
  GhostContext ghost_ctx {
    tile_map,
    ghosts_phase,
    entities.player,
    entities.blinky,
    {13,14}, // pen door
    {13,17}  // pen home
  };

  // Detects window close button or ESC key
  while (!WindowShouldClose()) {
      const float dt = GetFrameTime();

    // Win condition
    if (entities.player.collected_dots >= tile_map.all_dots) {
      BeginDrawing();
      ClearBackground(RAYWHITE);
      draw_map_and_entities(tile_map, &entities, ghosts_phase.state, dt);
      DrawText(TextFormat("SCORE: %i", entities.player.collected_dots),
               10, 10, 20, MAROON);

      const char *msg = "YOU WON!";
      draw_end_game_text(msg, screen_width, screen_height);

      EndDrawing();
      continue;
    }

    // Lose condition
    if (entities.player.is_dead) {
      BeginDrawing();
      ClearBackground(RAYWHITE);

      draw_map_and_entities(tile_map, &entities, ghosts_phase.state, dt);
      DrawText(TextFormat("SCORE: %i", entities.player.collected_dots),
               10, 10, 20, MAROON);

      const char *msg = "YOU LOST!";
      draw_end_game_text(msg, screen_width, screen_height);

      EndDrawing();
      continue;
    }

    // Gameplay loop    
    if (IsKeyPressed(KEY_UP)) {
      entities.player.next_dir = MOVEMENT_DIR::UP;
    }
  
    if (IsKeyPressed(KEY_DOWN)) {
      entities.player.next_dir = MOVEMENT_DIR::DOWN;
    }
  
    if (IsKeyPressed(KEY_RIGHT)) { 
      entities.player.next_dir = MOVEMENT_DIR::RIGHT;
    }
  
    if (IsKeyPressed(KEY_LEFT)) {
      entities.player.next_dir = MOVEMENT_DIR::LEFT;
    }

    // Checks and resolves previous frames collisions. Doing it here
    // prevents visual artifacts on collisions, due to the interpolation
    // that's happening after an entity moves to a new tile.
    check_and_resolve_entity_collisions(&entities);

    update_player(&tile_map, &entities.player, dt);
    update_ghosts_global_sm(&ghosts_phase, entities.player.is_energized,
                            scatter_schedule, chase_schedule, dt);
    update_ghost(&entities.blinky, GHOST_TYPE::BLINKY, ghost_ctx, dt);
    update_ghost(&entities.pinky,  GHOST_TYPE::PINKY, ghost_ctx, dt);
    update_ghost(&entities.inky,   GHOST_TYPE::INKY, ghost_ctx, dt);
    update_ghost(&entities.clyde,  GHOST_TYPE::CLYDE, ghost_ctx, dt);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    draw_map_and_entities(tile_map, &entities, ghosts_phase.state, dt);

    DrawText(TextFormat("SCORE: %i", entities.player.collected_dots),
             10, 10, 20, MAROON);
    
    EndDrawing();
  }

  // cleanup
  CloseWindow();
  return 0;
}

template<std::uint16_t Rows>
static std::pair<std::unique_ptr<TileMap>, std::unique_ptr<Entities>>
parse_level(const std::array<std::string, Rows>& level, std::uint16_t tile_size) {
  const std::uint16_t cols = static_cast<std::uint16_t>(level[0].size());
  const std::uint16_t rows = Rows;

  // Even though this is just a map of enums let's keep it on the heap,
  // thus freeing ourselves from having to specify its size during compile tima
  auto map = std::make_unique<TileMap>();
  map->tile_size = tile_size;
  map->rows = rows;
  map->cols = cols;
  map->tiles = std::make_unique<TILE_TYPE[]>(cols * rows);
  // Load and create all tile structures we'll need
  TILE_TYPE wall_tile     = TILE_TYPE::WALL;
  TILE_TYPE door_tile     = TILE_TYPE::DOOR;
  TILE_TYPE dot_tile      = TILE_TYPE::DOT;
  TILE_TYPE pill_tile     = TILE_TYPE::PILL;
  TILE_TYPE empty_tile    = TILE_TYPE::EMPTY;
  TILE_TYPE teleport_tile = TILE_TYPE::TELEPORT;

  auto entities = std::make_unique<Entities>();

  for (std::uint16_t row = 0; row < rows; ++row) {
    for (std::uint16_t col = 0; col < cols; ++col) {
      const char ch = level[row][col];

      switch (ch) {
      case '#': {
        map->set(col, row, wall_tile);
      } break;
      case '-': {
        map->set(col, row, door_tile);
      } break;
      case '.': {
        map->set(col, row, dot_tile);
      } break;
      case 'O': {
        map->set(col, row, pill_tile);
      } break;
      case ' ': {
        map->set(col, row, empty_tile);
      } break;
      case '=': {
        map->set(col, row, teleport_tile);
      } break;
      case 'P': {
        // Player/Pacman
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_entity(&entities->player, tile_pos, "resources/pacman_texture.png", 0.13f); // movement speed of ~8 tiles/sec
        map->set(col, row, empty_tile);
      } break;
      case 'B': {
        // Blinky
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_entity(&entities->blinky, tile_pos,
                    "resources/blinky_spritesheet.png", 0.2f); // movement speed of 5 tiles/sec
        entities->blinky.in_monster_pen = false;
        map->set(col, row, empty_tile);
      } break;
      case 'I': {
        // Inky
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_entity(&entities->inky, tile_pos, "resources/inky_spritesheet.png", 0.2f);
        map->set(col, row, empty_tile);
      } break;
      case 'K': {
        // Pinky (avoid P clash with Player). In our case Pinky is actually green :/
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_entity(&entities->pinky, tile_pos, "resources/pinky_spritesheet.png", 0.2f);
        map->set(col, row, empty_tile);
      } break;
      case 'C': {
        // Clyde
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_entity(&entities->clyde, tile_pos, "resources/clyde_spritesheet.png", 0.2f);
        map->set(col, row, empty_tile);
      } break;
      default: {
        map->set(col, row, empty_tile);
      } break;
      }
    }
  }

  return { std::move(map), std::move(entities) };
}

static void check_and_resolve_entity_collisions(Entities* entities) {
  Entity* player = &entities->player;
  Entity* ghosts[] = { &entities->blinky, &entities->pinky,
                       &entities->inky, &entities->clyde };

  for (Entity* ghost : ghosts) {
    if (ghost->is_dead) continue;

    if (entity_collision(*player, *ghost)) {
      if (player->is_energized) {
        ghost->is_dead = true;
      } else {
        player->is_dead = true;
        return;
      }
    }
  }
}

static void draw_map_and_entities(const TileMap& tile_map, Entities* entities,
                                  GHOST_STATE curr_ghost_state, float dt) {
  const std::uint16_t tile_size = tile_map.tile_size;
  const std::uint16_t total_rows = tile_map.rows;
  const std::uint16_t total_cols = tile_map.cols;
  const std::size_t total_tiles = static_cast<std::size_t>(total_rows) * total_cols;
  const TILE_TYPE* tile_data = tile_map.tiles.get();

  // Screen-space coordinates
  int pixel_x = 0;
  int pixel_y = 0;
  int pixel_center_x = tile_size / 2;
  int pixel_center_y = tile_size / 2;
  std::uint16_t current_col = 0;

  for (std::size_t tile_index = 0; tile_index < total_tiles; ++tile_index) {
    TILE_TYPE current_tile = tile_data[tile_index];

    if (current_tile == TILE_TYPE::WALL) {
      DrawRectangle(pixel_x, pixel_y, tile_size, tile_size, GREEN);
    }
    else if (current_tile == TILE_TYPE::DOT) {
      DrawCircle(pixel_center_x, pixel_center_y, 3, MAROON);
    }
    else if (current_tile == TILE_TYPE::PILL) {
      DrawCircle(pixel_center_x, pixel_center_y, 8, MAROON);
    }

    // Advance column/pixel without division to get the tile center
    pixel_x += tile_size;
    pixel_center_x += tile_size;
    ++current_col;

    if (current_col == total_cols) {
      current_col = 0;
      pixel_x = 0;
      pixel_center_x = tile_size / 2;
      pixel_y += tile_size;
      pixel_center_y += tile_size;
    }
  }

  // We use WHITE tint when we don't want any tint
  render_entity(tile_map, &entities->player, WHITE, dt);

  // Get a color with  30% opacity, for ghosts when dead
  Color dead_ghost_tint = WHITE;
  dead_ghost_tint.a = static_cast<unsigned char>(255 * 0.3f);

  Entity* ghosts[] = {
    &entities->blinky,
    &entities->pinky,
    &entities->inky,
    &entities->clyde
  };

  for (Entity* ghost : ghosts) {
    Color tint = WHITE;
    if (ghost->is_dead) {
      tint = dead_ghost_tint;
    } else if (curr_ghost_state == GHOST_STATE::FRIGHTENED) {
      tint = DARKBLUE;
    }

    render_entity(tile_map, ghost, tint, dt);
  }
}

static void draw_end_game_text(const char* msg,
                               std::uint32_t screen_width,
                               std::uint32_t screen_height) {
  int font_size = 60;

  int text_width = MeasureText(msg, font_size);
  int text_height = font_size;

  DrawText(msg, screen_width / 2 - text_width / 2,
           screen_height / 2 - text_height / 2, font_size, BLACK);
}
