/**
 * @file evaluate.cpp
 * @brief Tapered (opening / middlegame / endgame) evaluation.
 *
 * Each piece has three PSTs. The board incrementally tracks three scores
 * (PhaseScore) as moves are made; at eval time we compute a phase value
 * from remaining non-pawn material and blend the three scores linearly:
 *
 *   phase = 4*Q + 2*R + B + N (capped at 24, all pieces == 24)
 *   op_w  = max(0, phase - 16)   // 0..8, 8 when phase == 24
 *   eg_w  = max(0, 8 - phase)    // 0..8, 8 when phase == 0
 *   mg_w  = 8 - op_w - eg_w      // 0..8, 8 for phase in [8, 16]
 *   eval  = (op*op_w + mg*mg_w + eg*eg_w) / 8
 *
 * Tables are written from white's perspective. Row 0 = rank 1, row 7 = rank 8.
 * Files are symmetric in all tables so file ordering does not affect values.
 *
 * @author Anshuman Routray
 */

#include "evaluate.hpp"
#include "move_encoding.hpp"
#include <algorithm>
#include <cstdint>

// ---------------------------------------------------------------------------
// Material values (centipawns). Kept constant across phases; tune later.
// ---------------------------------------------------------------------------
static constexpr int16_t PIECE_VAL[6] = {0, 100, 320, 330, 500, 900};

// ---------------------------------------------------------------------------
// Phase weights per non-pawn piece. Sum at start: 4*4 + 2*4 + 2*1 + 2*1 = 24.
// ---------------------------------------------------------------------------
static constexpr int PHASE_OPEN_THRESHOLD = 16;
static constexpr int PHASE_END_THRESHOLD = 8;
static constexpr int PHASE_MAX = 24;

// ---------------------------------------------------------------------------
// PAWN
// Opening: reward central pawns moving to d4/e4, mild penalty for staying on d2/e2.
// Middlegame: classic central control + rank 7 push bonus.
// Endgame: heavy advancement reward — promotion is the goal.
// ---------------------------------------------------------------------------
static constexpr int16_t PAWN_OP_PST[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,   // rank 1
    5,   5,  -5, -10, -10,  -5,   5,   5,   // rank 2
    0,   0,   5,  15,  15,   5,   0,   0,   // rank 3
    0,   0,  10,  25,  25,  10,   0,   0,   // rank 4
    5,   5,  15,  25,  25,  15,   5,   5,   // rank 5
   10,  15,  20,  25,  25,  20,  15,  10,   // rank 6
   30,  30,  30,  30,  30,  30,  30,  30,   // rank 7
    0,   0,   0,   0,   0,   0,   0,   0,   // rank 8
};

static constexpr int16_t PAWN_MG_PST[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    5,  10,  10, -20, -20,  10,  10,   5,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    5,   5,  10,  25,  25,  10,   5,   5,
   10,  10,  20,  30,  30,  20,  10,  10,
   50,  50,  50,  50,  50,  50,  50,  50,
    0,   0,   0,   0,   0,   0,   0,   0,
};

static constexpr int16_t PAWN_EG_PST[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,  -5,  -5,   0,   0,   0,
    5,   5,   5,  10,  10,   5,   5,   5,
   10,  10,  15,  20,  20,  15,  10,  10,
   25,  25,  30,  35,  35,  30,  25,  25,
   55,  55,  60,  65,  65,  60,  55,  55,
  100, 100, 110, 110, 110, 110, 100, 100,
    0,   0,   0,   0,   0,   0,   0,   0,
};

// ---------------------------------------------------------------------------
// KNIGHT
// Opening: severe rim penalty, develop to f3 / c3 quickly.
// Middlegame: standard centralization, outpost squares rewarded.
// Endgame: same shape, slightly relaxed.
// ---------------------------------------------------------------------------
static constexpr int16_t KNIGHT_OP_PST[64] = {
  -60, -50, -40, -40, -40, -40, -50, -60,
  -40, -25,  -5,   0,   0,  -5, -25, -40,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -30,   0,  15,  20,  20,  15,   0, -30,
  -30,   5,  15,  20,  20,  15,   5, -30,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -40, -20,   0,   0,   0,   0, -20, -40,
  -50, -40, -30, -30, -30, -30, -40, -50,
};

static constexpr int16_t KNIGHT_MG_PST[64] = {
  -50, -40, -30, -30, -30, -30, -40, -50,
  -40, -20,   0,   5,   5,   0, -20, -40,
  -30,   5,  10,  15,  15,  10,   5, -30,
  -30,   0,  15,  20,  20,  15,   0, -30,
  -30,   5,  15,  20,  20,  15,   5, -30,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -40, -20,   0,   0,   0,   0, -20, -40,
  -50, -40, -30, -30, -30, -30, -40, -50,
};

static constexpr int16_t KNIGHT_EG_PST[64] = {
  -50, -40, -30, -30, -30, -30, -40, -50,
  -40, -20,   0,   0,   0,   0, -20, -40,
  -30,   0,  10,  15,  15,  10,   0, -30,
  -30,   5,  15,  20,  20,  15,   5, -30,
  -30,   0,  15,  20,  20,  15,   0, -30,
  -30,   5,  10,  15,  15,  10,   5, -30,
  -40, -20,   0,   5,   5,   0, -20, -40,
  -50, -40, -30, -30, -30, -30, -40, -50,
};

// ---------------------------------------------------------------------------
// BISHOP
// Opening: reward central diagonals, penalty for corners and back rank.
// Middlegame: same shape with slightly stronger central control.
// Endgame: long diagonals dominate; mild centralization.
// ---------------------------------------------------------------------------
static constexpr int16_t BISHOP_OP_PST[64] = {
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,   5,   0,   0,   0,   0,   5, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   0,  10,  15,  15,  10,   0, -10,
  -10,   5,   5,  10,  10,   5,   5, -10,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10, -10, -10, -10, -10, -20,
};

static constexpr int16_t BISHOP_MG_PST[64] = {
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,   5,   0,   0,   0,   0,   5, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   0,  10,  10,  10,  10,   0, -10,
  -10,   5,   5,  10,  10,   5,   5, -10,
  -10,   0,   5,  10,  10,   5,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10, -10, -10, -10, -10, -20,
};

static constexpr int16_t BISHOP_EG_PST[64] = {
  -15, -10, -10, -10, -10, -10, -10, -15,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -10,   0,  10,  10,  10,  10,   0, -10,
  -10,   5,  10,  15,  15,  10,   5, -10,
  -10,   0,  10,  15,  15,  10,   0, -10,
  -10,   5,  10,  10,  10,  10,   5, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -15, -10, -10, -10, -10, -10, -10, -15,
};

// ---------------------------------------------------------------------------
// ROOK
// Opening: stay near home until castled; mild bonus on central back-rank files.
// Middlegame: 7th-rank bonus, mild open-file preference baked in.
// Endgame: active rooks everywhere, strong rank-7 bonus.
// ---------------------------------------------------------------------------
static constexpr int16_t ROOK_OP_PST[64] = {
    0,   0,   5,  10,  10,   5,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   5,   5,   0,   0,   0,
};

static constexpr int16_t ROOK_MG_PST[64] = {
    0,   0,   0,   5,   5,   0,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0,
};

static constexpr int16_t ROOK_EG_PST[64] = {
    0,   0,   5,   5,   5,   5,   0,   0,
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   5,  10,  10,  10,  10,   5,   0,
   20,  20,  20,  20,  20,  20,  20,  20,
    0,   0,   0,   0,   0,   0,   0,   0,
};

// ---------------------------------------------------------------------------
// QUEEN
// Opening: strong penalty for early development off home square.
// Middlegame: standard centralization with mild edge penalty.
// Endgame: very active, strong centralization.
// ---------------------------------------------------------------------------
static constexpr int16_t QUEEN_OP_PST[64] = {
  -20, -10, -10,  -5,  -5, -10, -10, -20,
  -10,   5,   0,   0,   0,   0,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
   -5,   0,   0,   0,   0,   0,   0,  -5,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10,  -5,  -5, -10, -10, -20,
};

static constexpr int16_t QUEEN_MG_PST[64] = {
  -20, -10, -10,  -5,  -5, -10, -10, -20,
  -10,   0,   5,   0,   0,   0,   0, -10,
  -10,   5,   5,   5,   5,   5,   0, -10,
    0,   0,   5,   5,   5,   5,   0,  -5,
   -5,   0,   5,   5,   5,   5,   0,  -5,
  -10,   0,   5,   5,   5,   5,   0, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -20, -10, -10,  -5,  -5, -10, -10, -20,
};

static constexpr int16_t QUEEN_EG_PST[64] = {
  -10,  -5,  -5,  -5,  -5,  -5,  -5, -10,
   -5,   0,   5,   5,   5,   5,   0,  -5,
   -5,   5,  10,  10,  10,  10,   5,  -5,
   -5,   5,  10,  15,  15,  10,   5,  -5,
   -5,   5,  10,  15,  15,  10,   5,  -5,
   -5,   5,  10,  10,  10,  10,   5,  -5,
   -5,   0,   5,   5,   5,   5,   0,  -5,
  -10,  -5,  -5,  -5,  -5,  -5,  -5, -10,
};

// ---------------------------------------------------------------------------
// KING
// Opening: hide on rank 1 corners (castled). Severe center penalty.
// Middlegame: same shape, slightly relaxed.
// Endgame: REVERSED — king must be central. This is the single biggest
// difference between phases and the main reason a non-tapered eval fails
// at endgames.
// ---------------------------------------------------------------------------
static constexpr int16_t KING_OP_PST[64] = {
   20,  40,  20,   0,   0,  20,  40,  20,
   20,  20,   0,   0,   0,   0,  20,  20,
  -10, -20, -20, -20, -20, -20, -20, -10,
  -20, -30, -30, -40, -40, -30, -30, -20,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
};

static constexpr int16_t KING_MG_PST[64] = {
   20,  30,  10,   0,   0,  10,  30,  20,
   20,  20,   0,   0,   0,   0,  20,  20,
  -10, -20, -20, -20, -20, -20, -20, -10,
  -20, -30, -30, -40, -40, -30, -30, -20,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
  -30, -40, -40, -50, -50, -40, -40, -30,
};

static constexpr int16_t KING_EG_PST[64] = {
  -50, -30, -30, -30, -30, -30, -30, -50,
  -30, -20, -10,   0,   0, -10, -20, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -20, -10,   0,   0, -10, -20, -30,
  -50, -40, -30, -20, -20, -30, -40, -50,
};

// ---------------------------------------------------------------------------
// Combined PST + material, indexed [phase][piece][square].
// ---------------------------------------------------------------------------
int16_t COMBINED_PST[3][7][64];

void init_eval_tables() {
  const int16_t *pst_op[7] = {nullptr,    PAWN_OP_PST, KNIGHT_OP_PST,
                              BISHOP_OP_PST, ROOK_OP_PST, QUEEN_OP_PST,
                              KING_OP_PST};
  const int16_t *pst_mg[7] = {nullptr,    PAWN_MG_PST, KNIGHT_MG_PST,
                              BISHOP_MG_PST, ROOK_MG_PST, QUEEN_MG_PST,
                              KING_MG_PST};
  const int16_t *pst_eg[7] = {nullptr,    PAWN_EG_PST, KNIGHT_EG_PST,
                              BISHOP_EG_PST, ROOK_EG_PST, QUEEN_EG_PST,
                              KING_EG_PST};
  const int16_t *const *tables[3] = {pst_op, pst_mg, pst_eg};
  for (int phase = 0; phase < 3; phase++) {
    for (int piece = PAWN_PIECE; piece <= KING_PIECE; piece++) {
      for (int sq = 0; sq < 64; sq++) {
        COMBINED_PST[phase][piece][sq] =
            (int16_t)(PIECE_VAL[piece] + tables[phase][piece][sq]);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Main evaluation entry point — blend the three incremental scores by phase.
// ---------------------------------------------------------------------------
int16_t evaluate(Board *board) {
  int phase = __builtin_popcountll(board->knights | board->enemy_knights) +
              __builtin_popcountll(board->bishops | board->enemy_bishops) +
              2 * __builtin_popcountll(board->rooks | board->enemy_rooks) +
              4 * __builtin_popcountll(board->queen | board->enemy_queen);
  if (phase > PHASE_MAX)
    phase = PHASE_MAX;

  int op_w = std::max(0, phase - PHASE_OPEN_THRESHOLD); // 0..8
  int eg_w = std::max(0, PHASE_END_THRESHOLD - phase);  // 0..8
  int mg_w = PHASE_END_THRESHOLD - op_w - eg_w;         // 0..8

  int blended = (board->pst_score_white.op * op_w +
                 board->pst_score_white.mg * mg_w +
                 board->pst_score_white.eg * eg_w) /
                PHASE_END_THRESHOLD;

  return (board->turn == WHITE_TURN) ? (int16_t)blended : (int16_t)-blended;
}

int16_t mvv_lva_heuristic(Board *board, Move move) {
  uint8_t from_location = GET_FROM_SQUARE(move);
  uint8_t to_location = GET_TO_SQUARE(move);
  uint8_t attacking_piece = board->piece_locations[from_location];
  uint8_t captured_piece = board->piece_locations[to_location];
  int16_t difference = 0;
  if (captured_piece != EMPTY_PIECE) {
    difference = PIECE_VAL[captured_piece] - PIECE_VAL[attacking_piece];
  }
  return difference;
}
