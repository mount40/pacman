#include "ghosts.h"
#include <cmath>
#include <array>
#include <algorithm>
#include "raylib.h"

static constexpr double SCATTER_DURS[4] = {7.0, 7.0, 5.0, 5.0};
static constexpr double CHASE_DURS[4]   = {20.0, 20.0, 20.0, std::numeric_limits<double>::infinity()};

static constexpr int prioritize_dir(MOVEMENT_DIR d) {
  switch (d) {
  case MOVEMENT_DIR::UP:    return 0;
  case MOVEMENT_DIR::LEFT:  return 1;
  case MOVEMENT_DIR::DOWN:  return 2;
  case MOVEMENT_DIR::RIGHT: return 3;
  default:                  return 4;
  }
}

static inline void start_scatter(GhostPhase& p) {
  p.state = GHOST_STATE::SCATTER;
  double d = SCATTER_DURS[std::min(p.cycle_idx, 3)];
  p.main_timer.set_duration(d);
  p.main_timer.start();
}

static inline void start_chase(GhostPhase& p) {
  p.state = GHOST_STATE::CHASE;
  double d = CHASE_DURS[std::min(p.cycle_idx, 3)];
  if (std::isfinite(d)) {
    p.main_timer.set_duration(d);
    p.main_timer.start();
  } else {
    if (p.main_timer.running()) p.main_timer.stop(); // infinite chase
  }
}

static inline
bool can_step_into_door(MOVEMENT_DIR dir, const Entity& g, bool fromInsidePen) {
  // Entering the pen is only from above (moving DOWN) and only when dead.
  if (dir == MOVEMENT_DIR::DOWN) return g.is_dead;

  // Allow leaving pen upward when alive
  if (dir == MOVEMENT_DIR::UP)   return fromInsidePen && !g.is_dead;
  return false;
}

static inline
bool is_walkable(TILE_TYPE dest, MOVEMENT_DIR dir, const Entity& g, bool fromInsidePen) {
  if (dest == TILE_TYPE::WALL)  return false;
  if (dest != TILE_TYPE::DOOR)  return true;
  return can_step_into_door(dir, g, fromInsidePen);
}

static inline
Vector2 get_scatter_target_tile_pos(GHOST_TYPE ghost,
                                    std::uint16_t tile_map_cols,
                                    std::uint16_t tile_map_rows) {
  switch (ghost) {
  case(GHOST_TYPE::BLINKY): {
    return Vector2{ static_cast<float>(tile_map_cols) - 2.0f, 0.0f };
  } break;
  case (GHOST_TYPE::PINKY): {
    return Vector2{ 2.0f, 0.0f };
  } break;
  case (GHOST_TYPE::INKY): {
    return Vector2{ static_cast<float>(tile_map_cols) - 2.0f,
                    static_cast<float>(tile_map_rows) - 1.0f };
  } break;
  case (GHOST_TYPE::CLYDE): {
    return Vector2{ 2.0f, static_cast<float>(tile_map_rows) - 1.0f };
  } break;
  default: {
    return Vector2{};
  } break;
  }
}

static Vector2 get_chase_target_tile_pos(GHOST_TYPE ghost,
                                         std::uint16_t tile_map_rows,
                                         const Vector2& player_tile_pos,
                                         MOVEMENT_DIR player_dir,
                                         const Vector2& blinky_tile_pos = {},
                                         const Vector2& clyde_tile_pos = {}) {
  switch (ghost) {
  case(GHOST_TYPE::BLINKY): {
    return player_tile_pos;
  } break;
  case (GHOST_TYPE::PINKY): {
    Vector2 delta = get_step_delta(player_dir);
    return { player_tile_pos.x + delta.x * 4.0f,
             player_tile_pos.y + delta.y * 4.0f };
  } break;
  case (GHOST_TYPE::INKY): {
    // Two ahead of player
    Vector2 delta = get_step_delta(player_dir);
    Vector2 two_ahead{ player_tile_pos.x + delta.x * 2.0f,
                       player_tile_pos.y + delta.y * 2.0f };
    // Vector from Blinky to that point
    Vector2 v{ two_ahead.x - blinky_tile_pos.x,
               two_ahead.y - blinky_tile_pos.y };
    // Double it
    return { two_ahead.x + v.x,
             two_ahead.y + v.y };
  } break;
  case (GHOST_TYPE::CLYDE): {
    float dist2 = Vector2DistanceSqr(clyde_tile_pos, player_tile_pos); 
    // if far, chase; if near, retreat to scatter corner
    if (dist2 >= 64.0f) // 8 tiles squared
      return player_tile_pos;
    return Vector2{ 2.0f, static_cast<float>(tile_map_rows) - 1.0f };
  } break;
  default: {
    return Vector2{};
  } break;
  }  
}

static void update_ghost_tile_pos(Entity* entity, MOVEMENT_DIR new_dir, float dt) {
  // entity->move_timer += dt;
  entity->move_timer += (1.0f / 60.0f);
  while (entity->move_timer >= entity->tile_step_time) {
    entity->move_timer -= entity->tile_step_time;

    // update direction only once per tile
    entity->dir = new_dir;

    // record previous tile BEFORE we step
    entity->prev_tile_pos = entity->tile_pos;
    Vector2 delta = get_step_delta(entity->dir);
    entity->tile_pos.x += delta.x;
    entity->tile_pos.y += delta.y;
  }
}

// NOTE: rename "blinky" to "ghost"
static void move_to_tile(TileMap& tile_map,
                         Entity* blinky, const Vector2& target_tile_pos,
                         MOVEMENT_DIR forbidden_dir, float dt) {
  if (forbidden_dir == MOVEMENT_DIR::STOPPED && blinky->dir != MOVEMENT_DIR::STOPPED) {
    forbidden_dir = get_opposite_dir(blinky->dir);
  }
  
  float& blinky_x = blinky->tile_pos.x;
  float& blinky_y = blinky->tile_pos.y;
  
  // NOTE: abstract into a function, used by both ghosts and player
  TILE_TYPE current_tile = tile_map.get(blinky_x, blinky_y);
  if (current_tile == TILE_TYPE::TELEPORT) {
    handle_entity_on_teleport_tile(blinky, tile_map.cols);
  }

  // Gather candidates
  PathTile candidates[4];
  size_t candidates_idx = 0;

  auto consider = [&](MOVEMENT_DIR dir) {
    if (dir == forbidden_dir) return;
    Vector2 delta = get_step_delta(dir);
    Vector2 pos   = { blinky_x + delta.x, blinky_y + delta.y };
    TILE_TYPE tile  = tile_map.get(pos.x, pos.y);

    bool from_inside_pen = blinky->in_monster_pen;

    float dist = Vector2DistanceSqr(pos, target_tile_pos);
    bool walk   = is_walkable(tile, dir, *blinky, from_inside_pen);
    candidates[candidates_idx++]   = PathTile{ dir, dist, walk };
  };

  consider(MOVEMENT_DIR::UP);
  consider(MOVEMENT_DIR::DOWN);
  consider(MOVEMENT_DIR::LEFT);
  consider(MOVEMENT_DIR::RIGHT);

  std::sort(candidates, candidates + candidates_idx,
            [](const PathTile& a, const PathTile& b) {
    if (a.walkable != b.walkable) return a.walkable;   // true before false
    if (a.dist != b.dist)       return a.dist < b.dist;
    return prioritize_dir(a.dir) < prioritize_dir(b.dir);
  });

  MOVEMENT_DIR new_dir = candidates_idx ? candidates[0].dir : MOVEMENT_DIR::RIGHT;
  update_ghost_tile_pos(blinky, new_dir, dt);
}

// // NOTE: make this data-driven
// void update_ghosts_phase(GhostPhase* curr_phase, bool is_player_energized, float dt) {
//   if (is_player_energized) {
//     curr_phase->state = GHOST_STATE::FRIGHTENED;
//     return;
//   }
  
//   if (curr_phase->state == GHOST_STATE::NONE) {
//     curr_phase->state = GHOST_STATE::SCATTER;
//     curr_phase->timer.set_duration(7.0);
//     curr_phase->timer.start();
//   }

//   // After the 4th scatter -> ghosts chase indefinetely
//   // NOTE: stop the clock running maybe
//   if ((curr_phase->state == GHOST_STATE::CHASE ||
//        curr_phase->state == GHOST_STATE::FRIGHTENED) &&
//       curr_phase->num_scatter_to_chase_trans > 4) {
//     // stop the watch if running
//     if (curr_phase->timer.running()) {
//       curr_phase->timer.stop();
//     }
//     return;
//   }

//   // We're in scatter and the timer has run out
//   if ((curr_phase->state == GHOST_STATE::SCATTER ||
//        curr_phase->state == GHOST_STATE::FRIGHTENED) &&
//       !curr_phase->timer.running()) {
//     curr_phase->state = GHOST_STATE::CHASE;
//     curr_phase->timer.set_duration(20.0);
//     curr_phase->timer.start();
//     curr_phase->num_scatter_to_chase_trans += 1;
//   }
  

//   // We're in chase and the timer has run out
//   if ((curr_phase->state == GHOST_STATE::CHASE ||
//        curr_phase->state == GHOST_STATE::FRIGHTENED) &&
//       !curr_phase->timer.running()) {
//     curr_phase->state = GHOST_STATE::SCATTER;

//     if (curr_phase->num_scatter_to_chase_trans > 2) {
//       curr_phase->timer.set_duration(5.0);
//     } else {
//       curr_phase->timer.set_duration(7.0);
//     }

//     curr_phase->timer.start();
//     curr_phase->num_chase_to_scatter_trans += 1;
//   }

//   // The timer is still running, advance it no matter the ghost state
//   if (curr_phase->timer.running()) {
//     curr_phase->timer.update(static_cast<double>(dt));
//     return;
//   }  
// }

void update_ghosts_phase(GhostPhase* phase, bool is_player_energized, float dt) {
  // Initialize first phase
  if (phase->state == GHOST_STATE::NONE) start_scatter(*phase);

  // Handle frightened overlay
  if (is_player_energized) {
    if (phase->state != GHOST_STATE::FRIGHTENED) {
      phase->prev_state = phase->state;
      phase->paused_remaining = phase->main_timer.remaining();
      if (phase->main_timer.running()) phase->main_timer.stop();
      phase->state = GHOST_STATE::FRIGHTENED;
      phase->change_seq++;
      phase->frightened_timer.set_duration(6.0);
      phase->frightened_timer.start();
    } else {
      phase->frightened_timer.update(dt);
    }
    return;
  }

  // Leaving frightened?
  if (phase->state == GHOST_STATE::FRIGHTENED) {
    // keep ticking frightened if someone calls with energized=false too early
    if (phase->frightened_timer.running()) phase->frightened_timer.update(dt);
    if (!phase->frightened_timer.running()) {
      // resume previous state and timer
      phase->state = (phase->prev_state == GHOST_STATE::NONE) ? GHOST_STATE::SCATTER : phase->prev_state;
      phase->change_seq++;
      if (std::isfinite(phase->paused_remaining) && phase->paused_remaining > 0.0) {
        phase->main_timer.set_duration(phase->paused_remaining);
        phase->main_timer.start();
      }
      phase->paused_remaining = 0.0;
    }
    return;
  }

  // Advance main timer
  if (phase->main_timer.running()) {
    phase->main_timer.update(dt);
    if (phase->main_timer.running()) return;
  }

  // Timer expired, toggle phase according to cycle table
  if (phase->state == GHOST_STATE::SCATTER) {
    phase->state = GHOST_STATE::CHASE;
    phase->change_seq++;
    start_chase(*phase);
  } else if (phase->state == GHOST_STATE::CHASE) {
    phase->cycle_idx = std::min(phase->cycle_idx + 1, 3);
    start_scatter(*phase);
  }
}

// NOTE: This seems like a good usecase for inheritance
void update_ghost(TileMap& tile_map, Entity* entity, GHOST_TYPE ghost,
                  const GhostPhase& phase, const Entity& player, float dt,
                  const Vector2 blinky_tile_pos, const Vector2 clyde_tile_pos) {
  // Start of the game, default to movement to the right
  if (entity->dir == MOVEMENT_DIR::STOPPED) {
    entity->dir = MOVEMENT_DIR::RIGHT;
  }

  Vector2 target_tile_pos = {};
  MOVEMENT_DIR forbidden_dir = MOVEMENT_DIR::STOPPED;

  if (entity->last_seen_change_seq != phase.change_seq) {
    forbidden_dir = entity->dir;
    entity->dir = get_opposite_dir(entity->dir);    
    entity->last_seen_change_seq = phase.change_seq;
  }

  if (phase.state == GHOST_STATE::SCATTER) {
    target_tile_pos = get_scatter_target_tile_pos(ghost, tile_map.cols, tile_map.rows);
  }

  if (phase.state == GHOST_STATE::CHASE) {
    target_tile_pos = get_chase_target_tile_pos(ghost,
                                                tile_map.rows,
                                                player.tile_pos,
                                                player.dir,
                                                blinky_tile_pos,
                                                clyde_tile_pos);
  }

  if (phase.state == GHOST_STATE::FRIGHTENED) {
    // when frightened ghosts turn around 180 degrees and run to a random tile
    target_tile_pos = Vector2{
      static_cast<float>(GetRandomValue(0, tile_map.cols - 1)),
      static_cast<float>(GetRandomValue(0, tile_map.rows - 1))
    };
  }

  // NOTE: Refactor this
  if (entity->in_monster_pen) {
    if (Vector2Equals(entity->tile_pos, Vector2{ 13.0f, 14.0f })) {
      entity->in_monster_pen = false;
    } else {
      // Gotta get out of that penitentiary first, eh?
      target_tile_pos = Vector2{ 13.0f, 14.0f };
    }
  }
  
  // Being dead overrides all other states
  if (entity->is_dead) {
    // NOTE: don't hardcode this position bellow
    target_tile_pos = Vector2{ 13.0f, 17.0f };
    // Ghost is inside the monster pen, ressurects are in order
    if (Vector2Equals(entity->tile_pos, target_tile_pos)) {
      entity->is_dead = false;
      entity->in_monster_pen = true;
    } 
  }
 
  move_to_tile(tile_map, entity, target_tile_pos, forbidden_dir, dt);
}
