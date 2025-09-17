#pragma once
#include <cstdint>
#include "raylib.h"
#include "timer.h"
#include "movement_dir.h"
#include "tile_map.h"

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
  bool is_dead;
  // Player specific
  std::uint16_t collected_dots;
  bool is_energized;
  Timer energized_timer;
  // Ghost specific
  bool in_monster_pen;
  std::uint32_t last_seen_change_seq{0};
};

inline bool entity_collision(const Entity& a, const Entity& b) {
  return a.tile_pos.x == b.tile_pos.x && a.tile_pos.y == b.tile_pos.y;
}

inline Vector2 get_tile_pos_ahead_of_entity(const Entity& e, int n) {
  Vector2 delta = get_step_delta(e.dir);
  return { e.tile_pos.x + delta.x*n, e.tile_pos.y + delta.y*n };
}

void handle_entity_on_teleport_tile(Entity* entity, std::uint16_t num_tile_map_cols);
void init_entity(Entity* player, const Vector2& tile_pos,
                 const char* texture_path, float movement_speed);
void render_entity(const TileMap& tile_map, Entity* entity, Color tint);
