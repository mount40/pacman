#include <cstdint>
#include <array>
#include <string>
#include <memory>
#include <utility>
// NOTE: remove this include
#include <cmath>

#include "raylib.h"
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

static void
init_ghost(Entity* player, const Vector2& tile_pos, const char* texture_path) {
  player->tile_pos = tile_pos;
  player->prev_tile_pos = tile_pos;
  player->dir = MOVEMENT_DIR::STOPPED;
  player->move_timer = 0.0f;
  player->tile_step_time = 0.2f; // movement speed of 5 tiles/sec

  // NOTE: Set to "resources/pacman_texture.png"
  player->texture = LoadTexture(texture_path);
  player->scale = Vector2{ 1.5f, 1.5f };
  player->rotation = 0.0f;

  // setup animation context
  player->anim_ctx = {};
  player->anim_ctx.frame_rec = {
    0.0f,
    0.0f,
    static_cast<float>(player->texture.width / 8),
    static_cast<float>(player->texture.height)
  };
  player->anim_ctx.current_frame = 0;
  player->anim_ctx.frames_counter = 0;

  // We assume 8 frames per sprite sheet, always
  player->anim_ctx.frames_speed = 8;
}

static void
render_ghost(const TileMap& tile_map, Entity* player) {
  const Texture2D player_texture = player->texture;
  const float tile_size = static_cast<float>(tile_map.tile_size);

  float alpha = Clamp(player->move_timer / player->tile_step_time, 0.0f, 1.0f);
  Vector2 interp_tile = {
    Lerp(player->prev_tile_pos.x, player->tile_pos.x, alpha),
    Lerp(player->prev_tile_pos.y, player->tile_pos.y, alpha)
  };

  // Calculate the new interpolated position and center it 
  Vector2 player_pos = {
    interp_tile.x * tile_size + (tile_size / 2),
    interp_tile.y * tile_size + (tile_size / 2)
  };
  
  player->anim_ctx.frames_counter++;

  // NOTE: maybe this shouldn't be a fixed timestep timer
  if (player->anim_ctx.frames_counter >= (60 / player->anim_ctx.frames_speed)) {
    player->anim_ctx.frames_counter = 0;
    player->anim_ctx.current_frame++;

    if (player->anim_ctx.current_frame > 5) player->anim_ctx.current_frame = 0;

    player->anim_ctx.frame_rec.x = static_cast<float>(player->anim_ctx.current_frame) * static_cast<float>(player_texture.width / 8);
  }

  Rectangle src = player->anim_ctx.frame_rec;
  const float frame_w = std::fabsf(src.width);
  const float frame_h = src.height;

  // NOTE: corner case, mirror purely in the source rect if scale.x is negative
  if (player->scale.y < 0.0f) {
    src.x += frame_w;   // shift to the right edge of the frame
    src.width = -frame_w;
  } else {
    src.width = frame_w;
  }

  src.width = frame_w;
  const float draw_w = frame_w * std::fabsf(player->scale.x);
  const float draw_h = frame_h * std::fabsf(player->scale.y);
 
  Rectangle dst = {
    player_pos.x,  // center X
    player_pos.y,  // center Y
    draw_w,
    draw_h
  };

  // Origin inside destination rect (0,0 = top-left; to center use half size)
  Vector2 origin = { draw_w * 0.5f, draw_h * 0.5f  };

  DrawTexturePro(player_texture, src, dst, origin, player->rotation, WHITE);
}


template<std::uint16_t Rows>
static std::pair<std::unique_ptr<TileMap>, std::unique_ptr<Entities>>
parse_level(const std::array<std::string, Rows>& level, std::uint16_t tile_size);

static void DrawMapAndEntities(const TileMap& tile_map, Entities* entities);

int main(void) {
  // Ultimately, tile size and map dimensions should be part of the tile map file
  const std::uint16_t tile_size = 24;
  const std::uint16_t num_tiles_x = 28;
  const std::uint16_t num_tiles_y = 36;
  const std::uint32_t screen_width = num_tiles_x * tile_size;
  const std::uint32_t screen_height = num_tiles_y * tile_size;

  // The map should also be part of the tile map file
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

  // init
  InitWindow(screen_width, screen_height, "Modern Pacman");
  SetTargetFPS(60);

  auto [tile_map_ptr, entities_ptr] = parse_level(level, tile_size);

  // Using these locals to avoid dereferencing like crazy
  TileMap& tile_map = *tile_map_ptr;
  Entities& entities = *entities_ptr;

  std::uint32_t collected_dots = 0;
  
  // detects window close button or ESC key
  while (!WindowShouldClose()) {
    if (collected_dots >= tile_map.dots) {
      BeginDrawing();
      ClearBackground(RAYWHITE);
      const char *msg = "YOU WON!";
      int fontSize = 60;

      int textWidth = MeasureText(msg, fontSize);
      int textHeight = fontSize;

      DrawMapAndEntities(tile_map, &entities);
      DrawText(TextFormat("SCORE: %i", collected_dots), 10, 10, 20, MAROON);
      DrawText(msg, screen_width / 2 - textWidth / 2,
               screen_height / 2 - textHeight / 2, fontSize, BLACK);

      EndDrawing();
      continue;
    }
    
    const float dt = GetFrameTime();
    
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

    update_player(tile_map, &entities.player, &collected_dots, dt);
    update_blinky(tile_map, &entities.blinky, entities.player, dt);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawMapAndEntities(tile_map, &entities);

    DrawFPS(940, 10);
    DrawText(TextFormat("SCORE: %i", collected_dots), 10, 10, 20, MAROON);
    
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

  // NOTE: maybe this map doesn't need to be on the heap
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
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_player(&entities->player, tile_pos);
        map->set(col, row, empty_tile);
      } break;
      case 'B': { // Blinky
          // NOTE: Set to "resources/pacman_texture.png"
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_ghost(&entities->blinky, tile_pos, "D:/Projects/modern-pacman/raylib-game-template/src/resources/blinky_spritesheet.png");
        map->set(col, row, empty_tile);
      } break;
      case 'I': { // Inky
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_ghost(&entities->inky, tile_pos, "D:/Projects/modern-pacman/raylib-game-template/src/resources/inky_spritesheet.png");
        map->set(col, row, empty_tile);
      } break;
      case 'K': {
        // Pinky (avoid P clash with Player). In our case Pinky is actually green :/
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_ghost(&entities->pinky, tile_pos, "D:/Projects/modern-pacman/raylib-game-template/src/resources/pinky_spritesheet.png");
        map->set(col, row, empty_tile);
      } break;
      case 'C': { // Clyde
        Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
        init_ghost(&entities->clyde, tile_pos, "D:/Projects/modern-pacman/raylib-game-template/src/resources/clyde_spritesheet.png");
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

static void DrawMapAndEntities(const TileMap& tile_map, Entities* entities) {
  std::uint16_t tile_size = tile_map.tile_size;

  // NOTE: this seems like it could be made a bit more cache friendly
  for (std::uint16_t row = 0; row < tile_map.rows; row++) {
    for (std::uint16_t col = 0; col < tile_map.cols; col++) {
      TILE_TYPE tile = tile_map.get(col, row);

      switch(tile) {
      case TILE_TYPE::WALL: {
        DrawRectangle(col * tile_size, row * tile_size, tile_size, tile_size, GREEN);
      } break;
      case TILE_TYPE::DOT: {
        DrawCircle(col * tile_size + (tile_size / 2), row * tile_size + (tile_size / 2), 3, MAROON);
      } break;
      case TILE_TYPE::PILL: {
        DrawCircle(col * tile_size + (tile_size / 2), row * tile_size + (tile_size / 2), 8, MAROON);
      } break;
      default: {
      } break;
      }
    }
  }

  render_player(tile_map, &entities->player);
  render_ghost(tile_map, &entities->blinky);
  render_ghost(tile_map, &entities->inky);
  render_ghost(tile_map, &entities->pinky);
  render_ghost(tile_map, &entities->clyde);
}
