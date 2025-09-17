#pragma once
#include <cstdint>
#include <utility>
#include <memory>
#include <string>
#include "tile_map.h"
#include "entity.h"

struct Entities {
	Entity player;
	Entity blinky;
	Entity pinky;
	Entity inky;
	Entity clyde;
};

template<std::uint16_t Rows>
std::pair<std::unique_ptr<TileMap>, std::unique_ptr<Entities>>
parse_level(const std::array<std::string, Rows>& level, std::uint16_t tile_size) {
    const std::uint16_t cols = static_cast<std::uint16_t>(level[0].size());
    const std::uint16_t rows = Rows;

    // Even though this is just a map of enums let's keep it on the heap,
    // thus freeing ourselves from having to specify its size during compile tima
    auto map = std::make_unique<TileMap>();
    map->tile_size = tile_size;
    map->rows = rows;
    map->cols = cols;
    map->tiles = std::make_unique<TILE_TYPE[]>(cols * rows);
    // Load and create all tile structures we'll need
    TILE_TYPE wall_tile = TILE_TYPE::WALL;
    TILE_TYPE door_tile = TILE_TYPE::DOOR;
    TILE_TYPE dot_tile = TILE_TYPE::DOT;
    TILE_TYPE pill_tile = TILE_TYPE::PILL;
    TILE_TYPE empty_tile = TILE_TYPE::EMPTY;
    TILE_TYPE teleport_tile = TILE_TYPE::TELEPORT;

    auto entities = std::make_unique<Entities>();

    for (std::uint16_t row = 0; row < rows; ++row) {
        for (std::uint16_t col = 0; col < cols; ++col) {
            const char ch = level[row][col];

            switch (ch) {
            case '#': {
                map->set(col, row, wall_tile);
            } break;
            case '-': {
                map->set(col, row, door_tile);
            } break;
            case '.': {
                map->set(col, row, dot_tile);
            } break;
            case 'O': {
                map->set(col, row, pill_tile);
            } break;
            case ' ': {
                map->set(col, row, empty_tile);
            } break;
            case '=': {
                map->set(col, row, teleport_tile);
            } break;
            case 'P': {
                // Player/Pacman
                Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
                init_entity(&entities->player, tile_pos, "resources/pacman_texture.png", 0.15f); // movement speed of ~10 tiles/sec
                map->set(col, row, empty_tile);
            } break;
            case 'B': {
                // Blinky
                Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
                init_entity(&entities->blinky, tile_pos,
                    "resources/blinky_spritesheet.png", 0.2f); // movement speed of 5 tiles/sec
                entities->blinky.in_monster_pen = false;
                map->set(col, row, empty_tile);
            } break;
            case 'I': {
                // Inky
                Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
                init_entity(&entities->inky, tile_pos, "resources/inky_spritesheet.png", 0.2f);
                map->set(col, row, empty_tile);
            } break;
            case 'K': {
                // Pinky (avoid P clash with Player). In our case Pinky is actually green :/
                Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
                init_entity(&entities->pinky, tile_pos, "resources/pinky_spritesheet.png", 0.2f);
                map->set(col, row, empty_tile);
            } break;
            case 'C': {
                // Clyde
                Vector2 tile_pos = Vector2{ static_cast<float>(col), static_cast<float>(row) };
                init_entity(&entities->clyde, tile_pos, "resources/clyde_spritesheet.png", 0.2f);
                map->set(col, row, empty_tile);
            } break;
            default: {
                map->set(col, row, empty_tile);
            } break;
            }
        }
    }

    return { std::move(map), std::move(entities) };
}
