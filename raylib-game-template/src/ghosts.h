#pragma once
#include <cmath>
#include <limits>
#include "tile_map.h"
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

struct GhostPhase {
  GHOST_STATE state;
  Timer timer;
  uint16_t num_scatter_to_chase_trans;
  uint16_t num_chase_to_scatter_trans;
};

static constexpr int prio(MOVEMENT_DIR d);
static void move_to_tile(TileMap& tile_map, Entity* blinky,
                         const Vector2& target_tile_pos, float dt);

void update_ghosts_phase(GhostPhase* curr_phase, float dt);
void update_blinky(TileMap& tile_map, Entity* blinky,
                   GHOST_STATE curr_state, const Entity& player, float dt);
