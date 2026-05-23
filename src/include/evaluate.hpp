#pragma once

#include "board_manager.hpp"
#include <cstdint>

extern int16_t COMBINED_PST[7][64];

int16_t evaluate(Board *board);
int16_t mvv_lva_heuristic(Board *board, Move move);
void init_eval_tables();

// White-perspective PST contribution for a piece of `color` at `sq`.
// color: 1 = white (no mirror, positive), 0 = black (mirror, negative).
inline int16_t pst_white_contribution(uint8_t color, uint8_t piece_type,
                                      uint8_t sq) {
  return color ? COMBINED_PST[piece_type][sq]
               : -COMBINED_PST[piece_type][sq ^ 56];
}
