#include "entity.h"

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
                 const char* texture_path, float tile_step_time) {
  entity->tile_pos = tile_pos;
  entity->prev_tile_pos = tile_pos;
  entity->dir = MOVEMENT_DIR::STOPPED;
  entity->move_timer = 0.0f;
  entity->tile_step_time = tile_step_time; // 0.2f; // movement speed of 5 tiles/sec

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
