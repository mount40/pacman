#include "entity.h"
#include "raylib.h"
#include "raymath.h"

void handle_entity_on_teleport_tile(Entity* entity,
                                    std::uint16_t num_tile_map_cols) {
  if (entity->tile_pos.x == 0.0f) {
    // leftmost tile, send to rightmost tile
    entity->tile_pos.x = num_tile_map_cols - 2.0f;
    entity->prev_tile_pos.x = entity->tile_pos.x;
  } else if (entity->tile_pos.x == static_cast<float>(num_tile_map_cols) - 1.0f) {
    // rightmost tile, send to leftmost tile
    entity->tile_pos.x = 1.0f;
    entity->prev_tile_pos.x = entity->tile_pos.x;
  }
}

void init_entity(Entity* entity, const Vector2& tile_pos,
                 const char* texture_path, float movement_speed) {
  entity->tile_pos = tile_pos;
  entity->prev_tile_pos = tile_pos;
  entity->dir = MOVEMENT_DIR::STOPPED;
  entity->move_timer = 0.0f;
  entity->tile_step_time = movement_speed;

  // NOTE: Set to "resources/pacman_texture.png"
  entity->texture = LoadTexture(texture_path);
  entity->scale = Vector2{ 1.5f, 1.5f };
  entity->rotation = 0.0f;

  // setup animation context
  entity->anim_ctx = {};
  entity->anim_ctx.frame_rec = {
    0.0f,
    0.0f,
    static_cast<float>(entity->texture.width / 8),
    static_cast<float>(entity->texture.height)
  };
  entity->anim_ctx.current_frame = 0;
  entity->anim_ctx.frames_counter = 0;

  // We assume 8 frames per sprite sheet, always
  entity->anim_ctx.frames_speed = 8;

  entity->is_energized = false;
  entity->energized_timer = Timer();
  entity->is_dead = false;
  entity->in_monster_pen = true;
}

void render_entity(const TileMap& tile_map, Entity* entity, Color tint) {
  const Texture2D player_texture = entity->texture;
  const float tile_size = static_cast<float>(tile_map.tile_size);

  float alpha = Clamp(entity->move_timer / entity->tile_step_time, 0.0f, 1.0f);
  Vector2 interp_tile = {
    Lerp(entity->prev_tile_pos.x, entity->tile_pos.x, alpha),
    Lerp(entity->prev_tile_pos.y, entity->tile_pos.y, alpha)
  };

  // Calculate the new interpolated position and center it 
  Vector2 player_pos = {
    interp_tile.x * tile_size + (tile_size / 2),
    interp_tile.y * tile_size + (tile_size / 2)
  };
  
  entity->anim_ctx.frames_counter++;

  // NOTE: maybe this shouldn't be a fixed timestep timer
  if (entity->anim_ctx.frames_counter >= (60 / entity->anim_ctx.frames_speed)) {
    entity->anim_ctx.frames_counter = 0;
    entity->anim_ctx.current_frame++;

    if (entity->anim_ctx.current_frame > 5) entity->anim_ctx.current_frame = 0;

    entity->anim_ctx.frame_rec.x = static_cast<float>(entity->anim_ctx.current_frame) * static_cast<float>(player_texture.width / 8);
  }

  Rectangle src = entity->anim_ctx.frame_rec;
  const float frame_w = std::fabsf(src.width);
  const float frame_h = src.height;

  // NOTE: corner case, mirror purely in the source rect if scale.x is negative
  if (entity->scale.y < 0.0f) {
    src.x += frame_w;   // shift to the right edge of the frame
    src.width = -frame_w;
  } else {
    src.width = frame_w;
  }

  // src.width = frame_w;
  const float draw_w = frame_w * std::fabsf(entity->scale.x);
  const float draw_h = frame_h * std::fabsf(entity->scale.y);
 
  Rectangle dst = {
    player_pos.x,  // center X
    player_pos.y,  // center Y
    draw_w,
    draw_h
  };

  // Origin inside destination rect (0,0 = top-left; to center use half size)
  Vector2 origin = { draw_w * 0.5f, draw_h * 0.5f  };

  DrawTexturePro(player_texture, src, dst, origin, entity->rotation, tint);
}
