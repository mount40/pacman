#pragma once
#include <cstdint>
#include <memory>

enum TILE_TYPE {
  EMPTY = 0,
  WALL,
  DOT,
  PILL,
  TELEPORT,
};

/* struct Tile { */
/*   TILE_TYPE type; */
/*   Texture2D texture; // This holds only the handle + some metadata */
/* }; */

struct TileMap {
  std::uint16_t tile_size;
  std::uint16_t rows;
  std::uint16_t cols;
  std::uint16_t dots;
  std::unique_ptr<TILE_TYPE[]> tiles;

  inline TILE_TYPE get(std::uint16_t col, std::uint16_t row) const {
    if ( (row * cols + col) > (cols * rows) ) return TILE_TYPE::EMPTY;
    return tiles[row * cols + col];
  }

  /* inline const Tile& get_tile(std::uint16_t col, std::uint16_t row) const { */
  /*   return tiles[row * cols + col]; */
  /* } */
  
  // NOTE: For convenience, double check if this is actually okay and we don't mislead future users of the struct with this API
  inline TILE_TYPE get(float col, float row) const {
    return get(static_cast<std::uint16_t>(col), static_cast<std::uint16_t>(row));
  }

  inline void set(std::uint16_t col, std::uint16_t row, TILE_TYPE tile) {
    if (tile == TILE_TYPE::DOT) dots += 1;
    tiles[row * cols + col] = tile;
  }

  inline void set(float col, float row, TILE_TYPE tile) {
    set(static_cast<std::uint16_t>(col), static_cast<std::uint16_t>(row), tile);
  }
};

