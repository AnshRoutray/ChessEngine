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

// ---------------------------------------------------------------------------
// Score accumulation: iterate set bits, add material + PST
// ---------------------------------------------------------------------------

// Mirror square for black pieces (flip rank)
static inline uint8_t mirror(uint8_t sq) { return sq ^ 56; }

static inline int16_t score_pieces(uint64_t bb, int16_t material,
                                   const int16_t pst[64], bool is_enemy) {
  int16_t score = 0;
  while (bb) {
    uint8_t sq = static_cast<uint8_t>(__builtin_ctzll(bb));
    bb &= bb - 1;
    score += material + pst[is_enemy ? mirror(sq) : sq];
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
  score += score_pieces(board->pawns, PIECE_VAL[PAWN_PIECE], PAWN_PST, false);
  score +=
      score_pieces(board->knights, PIECE_VAL[KNIGHT_PIECE], KNIGHT_PST, false);
  score +=
      score_pieces(board->bishops, PIECE_VAL[BISHOP_PIECE], BISHOP_PST, false);
  score += score_pieces(board->rooks, PIECE_VAL[ROOK_PIECE], ROOK_PST, false);
  score += score_pieces(board->queen, PIECE_VAL[QUEEN_PIECE], QUEEN_PST, false);
  score += score_pieces(board->king, 0, KING_PST, false);

  // Enemy pieces (negative contribution, mirrored PST)
  score -=
      score_pieces(board->enemy_pawns, PIECE_VAL[PAWN_PIECE], PAWN_PST, true);
  score -= score_pieces(board->enemy_knights, PIECE_VAL[KNIGHT_PIECE],
                        KNIGHT_PST, true);
  score -= score_pieces(board->enemy_bishops, PIECE_VAL[BISHOP_PIECE],
                        BISHOP_PST, true);
  score -=
      score_pieces(board->enemy_rooks, PIECE_VAL[ROOK_PIECE], ROOK_PST, true);
  score -=
      score_pieces(board->enemy_queen, PIECE_VAL[QUEEN_PIECE], QUEEN_PST, true);
  score -= score_pieces(board->enemy_king, 0, KING_PST, true);

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
