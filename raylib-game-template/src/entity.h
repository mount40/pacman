#pragma once
#include <cstdint>
#include "raylib.h"
#include "timer.h"

enum MOVEMENT_DIR {
  STOPPED = 0,
  UP,
  DOWN,
  RIGHT,
  LEFT
};

struct EntityAnimationContext {
  Rectangle frame_rec;
  std::uint32_t current_frame;
  std::uint32_t frames_counter;
  std::uint32_t frames_speed;
};

// Fat entity struct
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
};

inline bool are_on_same_tile(const Entity& a, const Entity& b) {
    return a.tile_pos.x == b.tile_pos.x && a.tile_pos.y == b.tile_pos.y;
}
