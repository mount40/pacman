#include "player.h"

#include <cmath>
#include "timer.h"

void init_player(Entity* player, const Vector2& tile_pos) {
  player->tile_pos = tile_pos;
  player->prev_tile_pos = tile_pos;

  player->dir = MOVEMENT_DIR::STOPPED;
  player->move_timer = 0.0f;
  player->tile_step_time = 0.15f; // NOTE: 15 is good, movement speed of 15 tiles/sec

  // NOTE: Set to "resources/pacman_texture.png"
  player->texture = LoadTexture("D:/Projects/modern-pacman/raylib-game-template/src/resources/pacman_texture.png");
  player->scale = Vector2{ 1.5f, 1.5f };
  player->rotation = 0.0f;

  // setup animation context
  player->anim_ctx = {};
  player->anim_ctx.frame_rec = {
    0.0f,
    0.0f,
    static_cast<float>(player->texture.width / 8),
    static_cast<float>(player->texture.height)
  };
  player->anim_ctx.current_frame = 0;
  player->anim_ctx.frames_counter = 0;

  // We assume 8 frames per sprite sheet, always
  player->anim_ctx.frames_speed = 8;

  player->is_energized = false;
  player->energized_timer = Timer();
}

void update_player(TileMap& tile_map, Entity* player, float dt) {
  float& player_x = player->tile_pos.x;
  float& player_y = player->tile_pos.y;

  TILE_TYPE current_tile = tile_map.get(player_x, player_y);

  if (current_tile == TILE_TYPE::DOT) {
    tile_map.set(player_x, player_y, TILE_TYPE::EMPTY);
    // NOTE: In the original pacman dots are worth 10 points
    player->collected_dots += 1;
  }

  if (current_tile == TILE_TYPE::PILL) {
    tile_map.set(player_x, player_y, TILE_TYPE::EMPTY);
    // NOTE: In the original pacman energizers are worth 50 points
    // player->collected_dots += 50;    
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
    if (player_x == 0.0f) {
      // leftmost tile, send to rightmost tile
      player_x = tile_map.cols - 2.0f;
      player->prev_tile_pos.x = player_x;
    } else if (player_x == static_cast<float>(tile_map.cols) - 1.0f) {
      // rightmost tile, send to leftmost tile
      player_x = 1.0f;
      player->prev_tile_pos.x = player_x;
    }
  }

  TILE_TYPE left_tile = tile_map.get(player_x - 1.0f, player_y);
  TILE_TYPE right_tile = tile_map.get(player_x + 1.0f, player_y);
  TILE_TYPE up_tile = tile_map.get(player_x, player_y - 1.0f);
  TILE_TYPE down_tile = tile_map.get(player_x, player_y + 1.0f);

  bool should_stop_moving_left  = (left_tile == TILE_TYPE::WALL &&
                                   player->dir == MOVEMENT_DIR::LEFT);
  bool should_stop_moving_right = (right_tile == TILE_TYPE::WALL &&
                                   player->dir == MOVEMENT_DIR::RIGHT);
  bool should_stop_moving_up    = (up_tile == TILE_TYPE::WALL &&
                                   player->dir == MOVEMENT_DIR::UP);
  // edge case: pacman cannot enter the monster pen, and can be entered only from top to bottom
  bool should_stop_moving_down  = ((down_tile == TILE_TYPE::WALL ||
                                    down_tile == TILE_TYPE::DOOR) &&
                                   player->dir == MOVEMENT_DIR::DOWN);
  bool should_stop_moving       = (should_stop_moving_left || should_stop_moving_right ||
                                   should_stop_moving_up   || should_stop_moving_down);
  if (should_stop_moving)
    player->dir = MOVEMENT_DIR::STOPPED;
  
  bool should_start_moving_left        = (left_tile != TILE_TYPE::WALL &&
                                          player->next_dir == MOVEMENT_DIR::LEFT);
  bool should_start_moving_right       = (right_tile != TILE_TYPE::WALL &&
                                          player->next_dir == MOVEMENT_DIR::RIGHT);
  bool should_start_moving_up          = (up_tile != TILE_TYPE::WALL &&
                                          player->next_dir == MOVEMENT_DIR::UP);
  bool should_start_moving_down        = (down_tile != TILE_TYPE::WALL &&
                                          down_tile != TILE_TYPE::DOOR &&
                                          player->next_dir == MOVEMENT_DIR::DOWN);
  bool should_start_moving_in_next_dir = (should_start_moving_left  ||
                                          should_start_moving_right ||
                                          should_start_moving_up    ||
                                          should_start_moving_down);

  if (should_start_moving_in_next_dir) {
    player->dir = player->next_dir;
    player->next_dir = MOVEMENT_DIR::STOPPED;
  }
  
  player->move_timer += dt;
  while (player->move_timer >= player->tile_step_time) {
    player->move_timer -= player->tile_step_time;

    // record previous tile BEFORE we step
    player->prev_tile_pos = player->tile_pos;

    switch(player->dir) {
    case (MOVEMENT_DIR::UP): {
      player->tile_pos.y -= 1.0f;
      player->rotation = 270.0f;
      player->scale.y = std::fabsf(player->scale.y);
    } break;
    case (MOVEMENT_DIR::DOWN): {
      player->tile_pos.y += 1.0f;
      player->rotation = 90.0f;
      player->scale.y = std::fabsf(player->scale.y);
    } break;
    case (MOVEMENT_DIR::RIGHT): {
      player->tile_pos.x += 1.0f;
      player->rotation = 0.0f;
      player->scale.y = std::fabsf(player->scale.y);
    } break;
    case (MOVEMENT_DIR::LEFT): {
      player->tile_pos.x -= 1.0f;
      player->rotation = 0.0f;
      player->scale.y = -std::fabsf(player->scale.y);
    } break;
    default: {
    } break;
    }
  }
}

void render_player(const TileMap& tile_map, Entity* player) {
  const Texture2D player_texture = player->texture;
  const float tile_size = static_cast<float>(tile_map.tile_size);

  float alpha = Clamp(player->move_timer / player->tile_step_time, 0.0f, 1.0f);
  Vector2 interp_tile = {
    Lerp(player->prev_tile_pos.x, player->tile_pos.x, alpha),
    Lerp(player->prev_tile_pos.y, player->tile_pos.y, alpha)
  };

  // Calculate the new interpolated position and center it 
  Vector2 player_pos = {
    interp_tile.x * tile_size + (tile_size / 2),
    interp_tile.y * tile_size + (tile_size / 2)
  };
  
  player->anim_ctx.frames_counter++;

  // NOTE: maybe this shouldn't a fixed timestep timer
  if (player->anim_ctx.frames_counter >= (60 / player->anim_ctx.frames_speed)) {
    player->anim_ctx.frames_counter = 0;
    player->anim_ctx.current_frame++;

    if (player->anim_ctx.current_frame > 5) player->anim_ctx.current_frame = 0;

    player->anim_ctx.frame_rec.x = static_cast<float>(player->anim_ctx.current_frame) * static_cast<float>(player_texture.width / 8);
  }

  Rectangle src = player->anim_ctx.frame_rec;
  const float frame_w = std::fabsf(src.width);
  const float frame_h = src.height;

  // NOTE: corner case, mirror purely in the source rect if scale.x is negative
  if (player->scale.y < 0.0f) {
    src.x += frame_w;   // shift to the right edge of the frame
    src.width = -frame_w;
  } else {
    src.width = frame_w;
  }

  const float draw_w = frame_w * std::fabsf(player->scale.x);
  const float draw_h = frame_h * std::fabsf(player->scale.y);
 
  Rectangle dst = {
    player_pos.x,  // center X
    player_pos.y,  // center Y
    draw_w,
    draw_h
  };

  // Origin inside destination rect (0,0 = top-left; to center use half size)
  Vector2 origin = { draw_w * 0.5f, draw_h * 0.5f  };

  DrawTexturePro(player_texture, src, dst, origin, player->rotation, WHITE);
}

