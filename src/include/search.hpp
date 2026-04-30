#pragma once

#include "board_manager.hpp"
#include <cstdint>

constexpr int16_t INF = INT16_MAX;
constexpr uint8_t MAX_CAPTURE_MOVES = 218;

constexpr int TT_SIZE = 1 << 25;

enum CUTOFF_FLAG : uint8_t { EXACT = 0, AT_LEAST = 1, AT_MOST = 2 };

struct TT_Entry {
  uint8_t depth = 0;
  CUTOFF_FLAG cutoff_flag = EXACT;
  Move best_move = 0;
  int16_t score = 0;
  uint64_t zobrist_hash = 0;
};

extern TT_Entry TT_TABLE[TT_SIZE];

Move retrieveBestMove(Board *board, uint8_t depth);

Move retrieveBestMoveAtDepth(Board *board, uint8_t depth);

int16_t stableSearch(Board *board, int16_t alpha, int16_t beta);

// HANDLE case where this function returns 0 (which means board has no moves
// left)
int16_t searchBestMove(Board *board, uint8_t depth, int16_t alpha,
                       int16_t beta);
