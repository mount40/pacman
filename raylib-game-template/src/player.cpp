#include "player.h"
#include <cstdint>
#include "tile_map.h"
#include "entity.h"
#include "timer.h"
#include "movement_dir.h"
#include "raymath.h"

void update_player(TileMap* tile_map, Entity* player, float dt) {
  float& player_x = player->tile_pos.x;
  float& player_y = player->tile_pos.y;

  TILE_TYPE current_tile = tile_map->get(player_x, player_y);

  // Collect any dots we land on
  if (current_tile == TILE_TYPE::DOT) {
    tile_map->set(player_x, player_y, TILE_TYPE::EMPTY);
    player->collected_dots += 1;
  }

  // Energize the player if he lands on a pill
  if (current_tile == TILE_TYPE::PILL) {
    tile_map->set(player_x, player_y, TILE_TYPE::EMPTY);
    player->is_energized = true;
    player->energized_timer.set_duration(6.0);
    player->energized_timer.start();
  }

  if (player->energized_timer.running()) {
    player->energized_timer.update(static_cast<double>(dt));
  } else {
    player->is_energized = false;
  }

  if (current_tile == TILE_TYPE::TELEPORT) {
    handle_entity_on_teleport_tile(player, tile_map->cols);
  }

  TILE_TYPE left_tile = tile_map->get(player_x - 1.0f, player_y);
  TILE_TYPE right_tile = tile_map->get(player_x + 1.0f, player_y);
  TILE_TYPE up_tile = tile_map->get(player_x, player_y - 1.0f);
  TILE_TYPE down_tile = tile_map->get(player_x, player_y + 1.0f);

  auto tile_ahead_of = [&](MOVEMENT_DIR dir) -> TILE_TYPE {
    switch (dir) {
    case MOVEMENT_DIR::LEFT:  return left_tile;
    case MOVEMENT_DIR::RIGHT: return right_tile;
    case MOVEMENT_DIR::UP:    return up_tile;
    case MOVEMENT_DIR::DOWN:  return down_tile;
    default:                  return current_tile;
    }
  };
  
  // Player passability: walls and doors block in any direction (player cannot enter monster pen)
  auto is_passable_for_player = [&](TILE_TYPE tile) -> bool {
    if (tile == TILE_TYPE::WALL) return false;
    if (tile == TILE_TYPE::DOOR) return false;
    return true;
  };

  // Stop if the tile ahead isn’t passable
  if (player->dir != MOVEMENT_DIR::STOPPED) {
    TILE_TYPE tile_ahead = tile_ahead_of(player->dir);
    if (!is_passable_for_player(tile_ahead)) {
      player->dir = MOVEMENT_DIR::STOPPED;
    }
  }

  // Start queued turn if destination is passable
  if (player->next_dir != MOVEMENT_DIR::STOPPED) {
    const TILE_TYPE tile_next = tile_ahead_of(player->next_dir);
    if (is_passable_for_player(tile_next)) {
      player->dir = player->next_dir;
      player->next_dir = MOVEMENT_DIR::STOPPED;
    }
  }

  // Advance player position to next tile
  player->move_timer += dt;
  while (player->move_timer >= player->tile_step_time) {
    player->move_timer -= player->tile_step_time;

    // record previous position BEFORE stepping
    player->prev_tile_pos = player->tile_pos;

    const Vector2 step_delta = get_step_delta(player->dir);
    player->tile_pos.x += step_delta.x;
    player->tile_pos.y += step_delta.y;

    player->rotation = get_dir_rotation(player->dir);
    player->scale.y = get_dir_scale_y_sign(player->dir) * std::fabsf(player->scale.y);
  }
}
