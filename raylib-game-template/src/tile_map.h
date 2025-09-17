#pragma once
#include <cstdint>
#include <cmath>
#include <memory>

enum TILE_TYPE {
  EMPTY = 0,
  WALL,
  DOOR,
  DOT,
  PILL,
  TELEPORT,
};

struct TileMap {
  std::uint16_t tile_size;
  std::uint16_t rows;
  std::uint16_t cols;
  std::uint16_t dots;
  std::unique_ptr<TILE_TYPE[]> tiles;

  inline bool in_bounds(std::uint16_t col, std::uint16_t row) const noexcept {
    return col < cols && row < rows;
  }

  inline std::size_t index(std::uint16_t col, std::uint16_t row) const noexcept {
    return std::size_t(row) * std::size_t(cols) + std::size_t(col);
  }

  inline TILE_TYPE get(std::uint16_t col, std::uint16_t row) const noexcept {
    return in_bounds(col, row) ? tiles[index(col, row)] : TILE_TYPE::EMPTY;
  }

  // Explicit float helpers — named to avoid confusion
  inline TILE_TYPE get(float col, float row) const noexcept {
    if (col < 0.f || row < 0.f) return TILE_TYPE::EMPTY;
    std::uint16_t int_col = static_cast<std::uint16_t>(std::floorf(col));
    std::uint16_t int_irow = static_cast<std::uint16_t>(std::floorf(row));
    return get(int_col, int_irow);
  }

  inline void set(std::uint16_t col, std::uint16_t row, TILE_TYPE tile) noexcept {
    if (!in_bounds(col, row)) return;
    auto& cell = tiles[index(col, row)];
    // Update dots count based on transition
    if (cell == TILE_TYPE::DOT && tile != TILE_TYPE::DOT) { 
        if (dots) --dots; 
    } else if (cell != TILE_TYPE::DOT && tile == TILE_TYPE::DOT) {
        ++dots; 
    }
    cell = tile;
  }

  inline void set(float col, float row, TILE_TYPE tile) noexcept {
    if (col < 0.f || row < 0.f) return;
    std::uint16_t int_col = static_cast<std::uint16_t>(std::floorf(col));
    std::uint16_t int_row = static_cast<std::uint16_t>(std::floorf(row));
    set(int_col, int_row, tile);
  }
};

