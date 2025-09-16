#pragma once
#include "raylib.h"

enum MOVEMENT_DIR {
  STOPPED = 0,
  UP,
  DOWN,
  RIGHT,
  LEFT,
};

struct EntityAnimationContext {
  Rectangle frame_rec;
  int current_frame;
  int frames_counter;
  int frames_speed;
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
};
