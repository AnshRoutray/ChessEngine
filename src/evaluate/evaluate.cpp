/**
 * @file evaluate.cpp
 * @brief Evaluation function for chess engine.
 *
 * Uses flat 64-entry PST arrays indexed by square (0-63, a1=0, h8=63).
 * All values are in centipawns. Positive = white advantage.
 * Enemy pieces use mirrored square index (sq ^ 56) so tables are
 * written from white's perspective only.
 *
 * @author Anshuman Routray
 */

#include "evaluate.hpp"
#include "move_encoding.hpp"
#include <iostream>

// ---------------------------------------------------------------------------
// Material values (centipawns)
// ---------------------------------------------------------------------------
static constexpr int16_t PIECE_VAL[6] = {0, 100, 320, 330, 500, 900};

// ---------------------------------------------------------------------------
// Piece-square tables (white perspective, a1=0 ... h8=63)
// Row 0 = rank 1, row 7 = rank 8. Each row is files a-h.
// ---------------------------------------------------------------------------

// Pawns: reward center control and advancement
static constexpr int16_t PAWN_PST[64] = {
    0,  0,  0,   0,   0,   0,   0,  0,  // rank 1 (never here)
    5,  10, 10,  -20, -20, 10,  10, 5,  // rank 2
    5,  -5, -10, 0,   0,   -10, -5, 5,  // rank 3
    0,  0,  0,   20,  20,  0,   0,  0,  // rank 4
    5,  5,  10,  25,  25,  10,  5,  5,  // rank 5
    10, 10, 20,  30,  30,  20,  10, 10, // rank 6
    50, 50, 50,  50,  50,  50,  50, 50, // rank 7
    0,  0,  0,   0,   0,   0,   0,  0,  // rank 8 (promotion)
};

// Knights: strongly prefer center, penalize rim
static constexpr int16_t KNIGHT_PST[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   5,   5,
    0,   -20, -40, -30, 5,   10,  15,  15,  10,  5,   -30, -30, 0,
    15,  20,  20,  15,  0,   -30, -30, 5,   15,  20,  20,  15,  5,
    -30, -30, 0,   10,  15,  15,  10,  0,   -30, -40, -20, 0,   0,
    0,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50,
};

// Bishops: prefer diagonals, avoid corners
static constexpr int16_t BISHOP_PST[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20, -10, 5,   0,   0,   0,
    0,   5,   -10, -10, 10,  10,  10,  10,  10,  10,  -10, -10, 0,
    10,  10,  10,  10,  0,   -10, -10, 5,   5,   10,  10,  5,   5,
    -10, -10, 0,   5,   10,  10,  5,   0,   -10, -10, 0,   0,   0,
    0,   0,   0,   -10, -20, -10, -10, -10, -10, -10, -10, -20,
};

// Rooks: reward 7th rank and open files
static constexpr int16_t ROOK_PST[64] = {
    0,  0,  0,  5,  5,  0,  0,  0,  -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0,  0,  0,  0,  0,  0,  -5, -5, 0, 0, 0, 0, 0, 0, -5,
    5,  10, 10, 10, 10, 10, 10, 5,  0,  0, 0, 0, 0, 0, 0, 0,
};

// Queens: avoid early development but prefer center when active
static constexpr int16_t QUEEN_PST[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   5,   0,  0,  0,   0,   -10,
    -10, 5,   5,   5,  5,  5,   0,   -10, 0,   0,   5,   5,  5,  5,   0,   -5,
    -5,  0,   5,   5,  5,  5,   0,   -5,  -10, 0,   5,   5,  5,  5,   0,   -10,
    -10, 0,   0,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20,
};

// King: strongly prefer castled position in middlegame
static constexpr int16_t KING_PST[64] = {
    20,  30,  10,  0,   0,   10,  30,  20,  20,  20,  0,   0,   0,
    0,   20,  20,  -10, -20, -20, -20, -20, -20, -20, -10, -20, -30,
    -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40,
    -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50,
    -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
};

// Combined PST + Material

static int16_t COMBINED_PST[7][64];

// ---------------------------------------------------------------------------
// Score accumulation: iterate set bits, add material + PST
// ---------------------------------------------------------------------------

// Precompute combined material and PST lookup tables

void init_eval_tables() {
  const int16_t *pst_tables[7] = {nullptr,  PAWN_PST,  KNIGHT_PST, BISHOP_PST,
                                  ROOK_PST, QUEEN_PST, KING_PST};
  for (int piece = PAWN_PIECE; piece <= KING_PIECE; piece++) {
    for (int sq = 0; sq < 64; sq++) {
      COMBINED_PST[piece][sq] = PIECE_VAL[piece] + pst_tables[piece][sq];
    }
  }
}

// Mirror square for enemy pieces (flip rank)
static inline uint8_t mirror(uint8_t sq) { return sq ^ 56; }

static inline int16_t score_pieces(uint64_t bb, uint8_t piece_type,
                                   bool is_enemy) {
  int16_t score = 0;
  while (bb) {
    uint8_t sq = __builtin_ctzll(bb);
    bb &= bb - 1;
    score += COMBINED_PST[piece_type][is_enemy ? mirror(sq) : sq];
  }
  return score;
}

// ---------------------------------------------------------------------------
// Main evaluation entry point
// ---------------------------------------------------------------------------

int16_t evaluate(Board *board) {
  int16_t score = 0;
  // std::cout << "Pawns: " << board->pawns << std::endl << "Knights: " <<
  //  board->knights << std::endl;
  // Friendly pieces (positive contribution)
  score += score_pieces(board->pawns, PAWN_PIECE, false);
  score += score_pieces(board->knights, KNIGHT_PIECE, false);
  score += score_pieces(board->bishops, BISHOP_PIECE, false);
  score += score_pieces(board->rooks, ROOK_PIECE, false);
  score += score_pieces(board->queen, QUEEN_PIECE, false);
  score += score_pieces(board->king, KING_PIECE, false);

  // Enemy pieces (negative contribution, mirrored PST)
  score -= score_pieces(board->enemy_pawns, PAWN_PIECE, true);
  score -= score_pieces(board->enemy_knights, KNIGHT_PIECE, true);
  score -= score_pieces(board->enemy_bishops, BISHOP_PIECE, true);
  score -= score_pieces(board->enemy_rooks, ROOK_PIECE, true);
  score -= score_pieces(board->enemy_queen, QUEEN_PIECE, true);
  score -= score_pieces(board->enemy_king, KING_PIECE, true);

  return score;
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
