#pragma once
#include <cstdint>
#include "raylib.h"

enum class MOVEMENT_DIR : std::uint8_t {
  STOPPED = 0,
  UP,
  DOWN,
  RIGHT,
  LEFT
};

// Basically MOVEMENT_DIR negate
inline MOVEMENT_DIR get_opposite_dir(MOVEMENT_DIR dir) {
  switch (dir) {
  case MOVEMENT_DIR::UP:    return MOVEMENT_DIR::DOWN;
  case MOVEMENT_DIR::DOWN:  return MOVEMENT_DIR::UP;
  case MOVEMENT_DIR::LEFT:  return MOVEMENT_DIR::RIGHT;
  case MOVEMENT_DIR::RIGHT: return MOVEMENT_DIR::LEFT;
  default:                  return MOVEMENT_DIR::STOPPED;
  }
}

// Helper for rotating based on MOVEMENT_DIR.
// IMPORTANT: In the case of MOVEMENT_DIR::LEFT we don't rotate but flip the whole sprite by negating the Y scale.
// See the helper function "get_dir_scale_y_sign".
inline float get_dir_rotation(MOVEMENT_DIR dir) {

  switch (dir) {
  case MOVEMENT_DIR::UP:    return 270.0f;
  case MOVEMENT_DIR::DOWN:  return  90.0f;
  case MOVEMENT_DIR::RIGHT: return   0.0f;
  case MOVEMENT_DIR::LEFT:  return   0.0f;
  default:                  return   0.0f;
  }
}

// Helper for LEFT direction rotations
inline float get_dir_scale_y_sign(MOVEMENT_DIR dir) {
    return (dir == MOVEMENT_DIR::LEFT) ? -1.0f : 1.0f;
}

// Get the positional delta in a given direction
inline Vector2 get_step_delta(MOVEMENT_DIR dir) {
  switch (dir) {
  case MOVEMENT_DIR::UP:    return { 0.0f, -1.0f };
  case MOVEMENT_DIR::DOWN:  return { 0.0f,  1.0f };
  case MOVEMENT_DIR::LEFT:  return {-1.0f,  0.0f };
  case MOVEMENT_DIR::RIGHT: return { 1.0f,  0.0f };
  default:                  return { 0.0f,  0.0f };
  }
}
