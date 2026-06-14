#pragma once

#include "board_manager.hpp"
#include <cstdint>

// COMBINED_PST[phase][piece][square]
// phase 0 = opening, 1 = middlegame, 2 = endgame.
// Material value is baked in alongside the PST.
extern int16_t COMBINED_PST[3][7][64];

int16_t evaluate(Board *board);
int16_t mvv_lva_heuristic(Board *board, Move move);
void init_eval_tables();

// White-perspective tapered PST contribution for a piece of `color` at `sq`.
// color: 1 = white (no mirror, positive), 0 = black (mirror, negative).
// Returns three scores, one per phase, so the caller can sum them
// incrementally and the blend happens later in evaluate().
inline PhaseScore pst_white_contribution(uint8_t color, uint8_t piece_type,
                                         uint8_t sq) {
  uint8_t idx = color ? sq : (uint8_t)(sq ^ 56);
  int16_t sign = color ? (int16_t)1 : (int16_t)-1;
  return PhaseScore{(int16_t)(sign * COMBINED_PST[0][piece_type][idx]),
                    (int16_t)(sign * COMBINED_PST[1][piece_type][idx]),
                    (int16_t)(sign * COMBINED_PST[2][piece_type][idx])};
}
