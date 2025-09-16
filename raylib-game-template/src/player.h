#pragma once
#include <cstdint>
#include "raylib.h"
#include "tile_map.h"
#include "entity.h"
#include "raymath.h"

void init_player(Entity* player, const Vector2& tile_pos);
void update_player(TileMap& tile_map, Entity* player,
                   std::uint32_t* collected_dots, float dt);
void render_player(const TileMap& tile_map, Entity* player);
