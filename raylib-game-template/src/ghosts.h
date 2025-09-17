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

struct GhostsStateMachine {
  GHOST_STATE state{GHOST_STATE::NONE};
  Timer main_timer;                         // for SCATTER/CHASE
  Timer frightened_timer;                   // overlay timer
  int cycle_idx{0};                         // 0..3 for the first 4 cycles
  double paused_remaining{0.0};             // remaining main time while frightened
  GHOST_STATE prev_state{GHOST_STATE::NONE};
  std::uint16_t change_seq{0};              // for ghosts to know if state changed
};

// Everything needed for a ghost update per frame
struct GhostContext {
  TileMap&        map;
  const GhostsStateMachine& phase;
  const Entity&   player;
  const Entity&   blinky;     // needed by Inky’s chase rule
  Vector2         pen_door;   // usually {13,14}
  Vector2         pen_home;   // usually {13,17}
};

void update_ghosts_global_sm(GhostsStateMachine* phase, bool is_player_energized,
                             const double scatter_schedule[],
                             const double chase_schedule[],
                             float dt);
void update_ghost(Entity* g, GHOST_TYPE type, const GhostContext& ctx, float dt);
