#pragma once
#include <cmath>
#include <limits>
#include "tile_map.h"
#include "entity.h"
#include "raymath.h"

struct PathTile {
  MOVEMENT_DIR dir;
  float dist;
  bool walkable;
};

static constexpr int prio(MOVEMENT_DIR d);
static void move_to_tile(TileMap& tile_map, Entity* blinky,
                         const Vector2& target_tile_pos, float dt);
void update_blinky(TileMap& tile_map, Entity* blinky, const Entity& player, float dt);
