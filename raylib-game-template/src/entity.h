#pragma once
#include <cstdint>
#include "raylib.h"
#include "timer.h"
#include "movement_dir.h"
#include "tile_map.h"

struct EntityAnimationContext {
  Rectangle frame_rec;
  std::uint32_t current_frame;
  float time_accum = 0.0f;
  std::uint32_t frames_speed;        // frames per second
};

// Fat entity struct
struct Entity {
  Vector2 tile_pos{};
  Vector2 prev_tile_pos{};
  MOVEMENT_DIR dir{MOVEMENT_DIR::STOPPED};
  MOVEMENT_DIR next_dir{MOVEMENT_DIR::STOPPED};
  float move_timer{0.0f};
  float tile_step_time{0.0f};

  // Rendering
  Texture2D texture{};
  Vector2 scale{1.5f, 1.5f};
  float rotation{0.0f};
  EntityAnimationContext anim_ctx{};

  // Gameplay flags
  bool is_dead{false};

  // Player gameplay specific
  std::uint16_t collected_dots{0};
  bool is_energized{false};
  Timer energized_timer{};

  // Ghost gameplay specific
  bool in_monster_pen{true};
  std::uint32_t last_seen_change_seq{0};
};

inline bool entity_collision(const Entity& a, const Entity& b) {
  return a.tile_pos.x == b.tile_pos.x && a.tile_pos.y == b.tile_pos.y;
}

inline Vector2 get_tile_pos_ahead_of_entity(const Entity& entity, int tiles) {
  Vector2 delta = get_step_delta(entity.dir);
  return { entity.tile_pos.x + delta.x * tiles, entity.tile_pos.y + delta.y * tiles };
}

void init_entity(Entity* player, const Vector2& tile_pos,
                 const char* texture_path, float movement_speed);
void render_entity(const TileMap& tile_map, Entity* entity, Color tint, float dt);
void handle_entity_on_teleport_tile(Entity* entity, std::uint16_t num_tile_map_cols);
