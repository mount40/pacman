#include "ghosts.h"
#include <cmath>
#include <array>
#include <algorithm>

static constexpr int prio(MOVEMENT_DIR d) {
    switch (d) {
        case MOVEMENT_DIR::UP:    return 0;
        case MOVEMENT_DIR::LEFT:  return 1;
        case MOVEMENT_DIR::DOWN:  return 2;
        case MOVEMENT_DIR::RIGHT: return 3;
        default:                  return 4;
    }
}

// NOTE: rename blinky to ghost
static void move_to_tile(TileMap& tile_map, Entity* blinky,
                         const Vector2& target_tile_pos, float dt) {
  MOVEMENT_DIR forbidden_dir = MOVEMENT_DIR::STOPPED;
  switch(blinky->dir) {
  case(MOVEMENT_DIR::UP): {
    forbidden_dir = MOVEMENT_DIR::DOWN;
  } break;
  case(MOVEMENT_DIR::DOWN): {
    forbidden_dir = MOVEMENT_DIR::UP;
  } break;
  case(MOVEMENT_DIR::LEFT): {
    forbidden_dir = MOVEMENT_DIR::RIGHT;
  } break;
  case(MOVEMENT_DIR::RIGHT): {
    forbidden_dir = MOVEMENT_DIR::LEFT;
  } break;
  default: { // we default to right movement
    forbidden_dir = MOVEMENT_DIR::LEFT;    
  } break;
  }

  float& blinky_x = blinky->tile_pos.x;
  float& blinky_y = blinky->tile_pos.y;

  // NOTE: optimize this, we already know forbidden_dir, use it to drop 1 tile computation
  TILE_TYPE left_tile = tile_map.get(blinky_x - 1.0f, blinky_y);
  TILE_TYPE right_tile = tile_map.get(blinky_x + 1.0f, blinky_y);
  TILE_TYPE up_tile = tile_map.get(blinky_x, blinky_y - 1.0f);
  TILE_TYPE down_tile = tile_map.get(blinky_x, blinky_y + 1.0f);

  Vector2 left_tile_pos = Vector2{ blinky_x - 1.0f, blinky_y };
  Vector2 right_tile_pos = Vector2{ blinky_x + 1.0f, blinky_y };
  Vector2 up_tile_pos = Vector2{ blinky_x, blinky_y - 1.0f };
  Vector2 down_tile_pos = Vector2{ blinky_x, blinky_y + 1.0f };

  // compute the distances
  float right_dist = Vector2DistanceSqr(right_tile_pos, target_tile_pos);
  float left_dist  = Vector2DistanceSqr(left_tile_pos, target_tile_pos);
  float up_dist    = Vector2DistanceSqr(up_tile_pos, target_tile_pos);
  float down_dist  = Vector2DistanceSqr(down_tile_pos, target_tile_pos);

  bool can_go_up    = up_tile != TILE_TYPE::WALL;
  // the door to the monster pen can be accessed only from top going down, and Blinky can enter the monster pen only when in EATEN state
  bool can_go_down  = down_tile != TILE_TYPE::WALL && down_tile != TILE_TYPE::DOOR;
  bool can_go_left  = left_tile != TILE_TYPE::WALL;
  bool can_go_right = right_tile != TILE_TYPE::WALL;

  std::array<PathTile, 4> a{{
      {MOVEMENT_DIR::UP, up_dist, can_go_up},
      {MOVEMENT_DIR::DOWN, down_dist, can_go_down},
      {MOVEMENT_DIR::LEFT, left_dist, can_go_left},
      {MOVEMENT_DIR::RIGHT, right_dist, can_go_right},
    }};

  // Drop away the forbidden direction
  std::size_t n = 0;
  for (const auto& c : a)
    if (c.dir != forbidden_dir) a[n++] = c;

  auto cmp = [](const PathTile& x, const PathTile& y) {
    if (x.walkable != y.walkable) return x.walkable;   // walkable first
    if (x.dist < y.dist) return true;
    if (y.dist < x.dist) return false;
    return prio(x.dir) < prio(y.dir);                  // UP > LEFT > DOWN > RIGHT
  };
  std::sort(a.begin(), a.begin() + n, cmp);

  // NOTE: take this out into a separate function per entity
  // blinky->move_timer += dt;
  blinky->move_timer += (1.0f / 60.0f);
  while (blinky->move_timer >= blinky->tile_step_time) {
    blinky->move_timer -= blinky->tile_step_time;

    // update direction only once per tile
    blinky->dir = n ? a[0].dir : MOVEMENT_DIR::RIGHT;

    // record previous tile BEFORE we step
    blinky->prev_tile_pos = blinky->tile_pos;


    switch(blinky->dir) {
    case (MOVEMENT_DIR::UP): {
      blinky->tile_pos.y -= 1.0f;
    } break;
    case (MOVEMENT_DIR::DOWN): {
      blinky->tile_pos.y += 1.0f;
    } break;
    case (MOVEMENT_DIR::RIGHT): {
      blinky->tile_pos.x += 1.0f;
    } break;
    case (MOVEMENT_DIR::LEFT): {
      blinky->tile_pos.x -= 1.0f;
    } break;
    default: {
    } break;
    }
  }
}

void update_ghosts_phase(GhostPhase* curr_phase, float dt) {
  if (curr_phase->state == GHOST_STATE::NONE) {
    curr_phase->state = GHOST_STATE::SCATTER;
    curr_phase->timer.set_duration(7.0);
    curr_phase->timer.start();
  }

  // After the 4th scatter -> ghosts chase indefinetely
  // NOTE: stop the clock running maybe
  if (curr_phase->state == GHOST_STATE::CHASE &&
      curr_phase->num_scatter_to_chase_trans > 4) {
    curr_phase->timer.stop();
    return;
  }

  // We're in scatter and the timer has run out
  if (curr_phase->state == GHOST_STATE::SCATTER &&
      !curr_phase->timer.running()) {
    curr_phase->state = GHOST_STATE::CHASE;
    curr_phase->timer.set_duration(20.0);
    curr_phase->timer.start();
    curr_phase->num_scatter_to_chase_trans += 1;
  }
  

  // We're in chase and the timer has run out
  if (curr_phase->state == GHOST_STATE::CHASE &&
      !curr_phase->timer.running()) {
    curr_phase->state = GHOST_STATE::SCATTER;

    if (curr_phase->num_scatter_to_chase_trans > 2) {
      curr_phase->timer.set_duration(5.0);
    } else {
      curr_phase->timer.set_duration(7.0);
    }

    curr_phase->timer.start();
    curr_phase->num_chase_to_scatter_trans += 1;
  }

  // The timer is still running, advance it no matter the ghost state
  if (curr_phase->timer.running()) {
    curr_phase->timer.update(static_cast<double>(dt));
    return;
  }  
}

void update_blinky(TileMap& tile_map, Entity* blinky, GHOST_STATE curr_state, const Entity& player, float dt) {
  // Start of the game, default to right movement
  if (blinky->dir == MOVEMENT_DIR::STOPPED) {
    blinky->dir = MOVEMENT_DIR::RIGHT;
  }

  Vector2 target_tile_pos = {};
  if (curr_state == GHOST_STATE::SCATTER)
    target_tile_pos = Vector2{ static_cast<float>(tile_map.cols) - 2.0f, 0.0f };
  else if (curr_state == GHOST_STATE::CHASE)
    target_tile_pos = Vector2{ player.tile_pos.x, player.tile_pos.y };

  move_to_tile(tile_map, blinky, target_tile_pos, dt);
}
