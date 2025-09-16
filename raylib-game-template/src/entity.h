#pragma once
#include <cstdint>
#include "raylib.h"
#include "timer.h"
#include "movement_dir.h"

struct EntityAnimationContext {
  Rectangle frame_rec;
  std::uint32_t current_frame;
  std::uint32_t frames_counter;
  std::uint32_t frames_speed;
};

// Fat entity struct
// NOTE: make player and ghost specific data fields into a union
struct Entity {
  Vector2 tile_pos;
  Vector2 prev_tile_pos; 
  MOVEMENT_DIR dir;
  MOVEMENT_DIR next_dir;
  float move_timer;
  float tile_step_time;
  Texture2D texture; // This holds only the handle + some metadata
  Vector2 scale;
  float rotation;
  EntityAnimationContext anim_ctx;
  std::uint16_t collected_dots;
  bool is_energized;
  Timer energized_timer;
  bool is_dead;
  bool in_monster_pen;
  std::uint32_t last_seen_change_seq{0};
};

inline bool entity_collision(const Entity& a, const Entity& b) {
  return a.tile_pos.x == b.tile_pos.x && a.tile_pos.y == b.tile_pos.y;
}

void handle_entity_on_teleport_tile(Entity* entity, std::uint16_t num_tile_map_cols);
void init_entity(Entity* player, const Vector2& tile_pos,
                 const char* texture_path, float tile_step_time);
