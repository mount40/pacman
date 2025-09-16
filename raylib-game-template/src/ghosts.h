#pragma once
#include "tile_map.h"
#include "entity.h"
#include "raymath.h"

void update_blinky(TileMap& tile_map, Entity* blinky, const Entity& player, float dt);
