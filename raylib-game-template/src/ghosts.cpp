#include "ghosts.h"
#include <algorithm>

static constexpr int prioritize_dir(MOVEMENT_DIR d) {
  switch (d) {
  case MOVEMENT_DIR::UP:    return 0;
  case MOVEMENT_DIR::LEFT:  return 1;
  case MOVEMENT_DIR::DOWN:  return 2;
  case MOVEMENT_DIR::RIGHT: return 3;
  default:                  return 4;
  }
}

static void start_scatter(GhostsStateMachine& sm, const double scatter_schedule[]) {
  sm.state = GHOST_STATE::SCATTER;
  double d = scatter_schedule[std::min(sm.cycle_idx, 3)];
  sm.main_timer.set_duration(d);
  sm.main_timer.start();
}

static void start_chase(GhostsStateMachine& sm, const double chase_schedule[]) {
  sm.state = GHOST_STATE::CHASE;
  double d = chase_schedule[std::min(sm.cycle_idx, 3)];
  if (std::isfinite(d)) {
    sm.main_timer.set_duration(d);
    sm.main_timer.start();
  } else {
    if (sm.main_timer.running()) sm.main_timer.stop(); // infinite chase
  }
}

static bool can_step_into_door(MOVEMENT_DIR dir, const Entity& ghost, bool from_inside_pen) {
  // Entering the pen is only from above (moving DOWN) and only when dead.
  if (dir == MOVEMENT_DIR::DOWN) return ghost.is_dead;

  // Allow leaving pen upward when alive
  if (dir == MOVEMENT_DIR::UP)   return from_inside_pen && !ghost.is_dead;
  return false;
}

static bool is_walkable(TILE_TYPE dest, MOVEMENT_DIR dir,
                        const Entity& ghost, bool from_inside_pen) {
  if (dest == TILE_TYPE::WALL)  return false;
  if (dest != TILE_TYPE::DOOR)  return true;
  return can_step_into_door(dir, ghost, from_inside_pen);
}

static Vector2 get_scatter_target(GHOST_TYPE ghost, const GhostContext& ctx){
  switch(ghost){
    case GHOST_TYPE::BLINKY: return { (float)ctx.map.cols - 2, 0 };
    case GHOST_TYPE::PINKY:  return { 2, 0 };
    case GHOST_TYPE::INKY:   return { (float)ctx.map.cols - 2, (float)ctx.map.rows - 1 };
    case GHOST_TYPE::CLYDE:  return { 2, (float)ctx.map.rows - 1 };
    default:                 return {};
  }
}

static Vector2 get_chase_target(GHOST_TYPE type, const Entity& ghost,
                                const GhostContext& ctx){
  switch(type){
    case GHOST_TYPE::BLINKY:
      return ctx.player.tile_pos;

    case GHOST_TYPE::PINKY: {
      Vector2 t = get_tile_pos_ahead_of_entity(ctx.player, 4); return t;
    }

    case GHOST_TYPE::INKY: {
      Vector2 two = get_tile_pos_ahead_of_entity(ctx.player, 2);
      Vector2 v{ two.x - ctx.blinky.tile_pos.x, two.y - ctx.blinky.tile_pos.y };
      return { two.x + v.x, two.y + v.y };
    }

    case GHOST_TYPE::CLYDE: {
      float dx = ctx.player.tile_pos.x - ghost.tile_pos.x;
      float dy = ctx.player.tile_pos.y - ghost.tile_pos.y;
      float d2 = Vector2DistanceSqr(ctx.player.tile_pos, ghost.tile_pos);
      return (d2 >= 64.0f) ? ctx.player.tile_pos : get_scatter_target(GHOST_TYPE::CLYDE, ctx);
    }
    default: return {};
  }
}

static void update_ghost_tile_pos(Entity* entity, MOVEMENT_DIR new_dir, float dt) {
  // entity->move_timer += (1.0f / 60.0f);
  entity->move_timer += dt;
  while (entity->move_timer >= entity->tile_step_time) {
    entity->move_timer -= entity->tile_step_time;

    // Update direction only once per tile
    entity->dir = new_dir;

    // Record previous tile BEFORE we step
    entity->prev_tile_pos = entity->tile_pos;
    Vector2 delta = get_step_delta(entity->dir);
    entity->tile_pos.x += delta.x;
    entity->tile_pos.y += delta.y;
  }
}

static void move_to_tile(TileMap& tile_map,
                         Entity* blinky, const Vector2& target_tile_pos,
                         MOVEMENT_DIR forbidden_dir, float dt) {
  if (forbidden_dir == MOVEMENT_DIR::STOPPED && blinky->dir != MOVEMENT_DIR::STOPPED) {
    forbidden_dir = get_opposite_dir(blinky->dir);
  }
  
  float& blinky_x = blinky->tile_pos.x;
  float& blinky_y = blinky->tile_pos.y;
  
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
    if (a.walkable != b.walkable) return a.walkable;   // check if walkable at all before else
    if (a.dist != b.dist)         return a.dist < b.dist;
    return prioritize_dir(a.dir) < prioritize_dir(b.dir);
  });

  MOVEMENT_DIR new_dir = candidates_idx ? candidates[0].dir : MOVEMENT_DIR::RIGHT;
  update_ghost_tile_pos(blinky, new_dir, dt);
}

void update_ghosts_global_sm(GhostsStateMachine* phase, bool is_player_energized,
                             const double scatter_schedule[],
                             const double chase_schedule[],
                             float dt) {
  // Initialize first phase
  if (phase->state == GHOST_STATE::NONE) start_scatter(*phase, scatter_schedule);

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
    start_chase(*phase, chase_schedule);
  } else if (phase->state == GHOST_STATE::CHASE) {
    phase->cycle_idx = std::min(phase->cycle_idx + 1, 3);
    start_scatter(*phase, scatter_schedule);
  }
}

void update_ghost(Entity* ghost, GHOST_TYPE type, const GhostContext& ctx, float dt) {
  if (ghost->dir == MOVEMENT_DIR::STOPPED) {
    ghost->dir = MOVEMENT_DIR::RIGHT;
  }

  MOVEMENT_DIR forbidden = MOVEMENT_DIR::STOPPED;

  // Reverse once per phase change
  if (ghost->last_seen_change_seq != ctx.phase.change_seq) {
    forbidden = ghost->dir;
    ghost->dir = get_opposite_dir(ghost->dir);
    ghost->last_seen_change_seq = ctx.phase.change_seq;
  }

  Vector2 target{};
  // Exit pen first
  if (ghost->in_monster_pen) {
    if (Vector2Equals(ghost->tile_pos, ctx.pen_door))
      ghost->in_monster_pen = false;
    else 
      target = ctx.pen_door;
  }

  // Being dead overrides all other states
  if (ghost->is_dead) {
    target = ctx.pen_home;
    if (Vector2Equals(ghost->tile_pos, ctx.pen_home)) {
      ghost->is_dead = false;
      ghost->in_monster_pen = true;
    }
  }

  // If no target tile is forced we can finally set the
  // target tile based on global state machine state 
  if (target.x == 0 && target.y == 0 &&
      !(ghost->is_dead || ghost->in_monster_pen)) {
    switch (ctx.phase.state) {
    case GHOST_STATE::SCATTER: {
      target = get_scatter_target(type, ctx);
    } break;
    case GHOST_STATE::CHASE: {
      target = get_chase_target(type, *ghost, ctx);
    } break;
    case GHOST_STATE::FRIGHTENED: {
      target = {
        (float)GetRandomValue(0, ctx.map.cols - 1),
        (float)GetRandomValue(0, ctx.map.rows - 1)
      };
    } break;
    default: break;
    }
  }

  move_to_tile(ctx.map, ghost, target, forbidden, dt);
}
