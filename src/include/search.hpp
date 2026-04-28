#include "board_manager.hpp"
#include <cstdint>

constexpr int16_t INF = INT16_MAX;
constexpr uint8_t MAX_CAPTURE_MOVES = 218;

Move retrieveBestMove(Board *board, uint8_t depth);

int16_t stableSearch(Board *board, int16_t alpha, int16_t beta);

// HANDLE case where this function returns 0 (which means board has no moves
// left)
int16_t searchBestMove(Board *board, uint8_t depth, int16_t alpha,
                       int16_t beta);
