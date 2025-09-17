#pragma once
#include <cmath>
#include <limits>
#include "tile_map.h"
#include "movement_dir.h"
#include "entity.h"
#include "raymath.h"
#include "timer.h"

struct PathTile {
  MOVEMENT_DIR dir;
  float dist;
  bool walkable;
};

enum class GHOST_STATE {
  NONE = 0,
  SCATTER,
  CHASE,
  FRIGHTENED,
  EATEN,
};

enum class GHOST_TYPE {
  NONE = 0,
  BLINKY,
  PINKY,
  INKY,
  CLYDE,
};

struct GhostPhase {
  GHOST_STATE state{GHOST_STATE::NONE};
  Timer main_timer;                         // for SCATTER/CHASE
  Timer frightened_timer;                   // overlay timer
  int cycle_idx{0};                         // 0..3 for the first 4 cycles
  double paused_remaining{0.0};             // remaining main time while frightened
  GHOST_STATE prev_state{GHOST_STATE::NONE};
  std::uint16_t change_seq{0};              // for ghosts to know if state changed
};

void update_ghosts_phase(GhostPhase* phase, bool is_player_energized, float dt);
void update_ghost(TileMap& tile_map, Entity* entity, GHOST_TYPE ghost,
                  const GhostPhase& phase, const Entity& player, float dt,
                  const Vector2 blinky_tile_pos, const Vector2 clyde_tile_pos);
