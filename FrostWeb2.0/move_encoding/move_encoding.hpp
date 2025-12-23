/**
 * Move Encoding (16 bits):
 *
 * Bits  0-5:  To square (0-63) (From bottom right)
 * Bits  6-11: From square (0-63) (From bottom right)
 * Bit   12:   En passant flag
 * Bits  13-15: Promotion piece (0=none, 1-4=N/B/R/Q)
 *
 */

#pragma once

#include <cstdint>

typedef uint16_t Move;

constexpr uint8_t KNIGHT_PIECE{1};
constexpr uint8_t BISHOP_PIECE{2};
constexpr uint8_t ROOK_PIECE{3};
constexpr uint8_t QUEEN_PIECE{4};

constexpr uint8_t TO_SQUARE_POS{0};
constexpr uint8_t FROM_SQUARE_POS{6};
constexpr uint8_t EN_PASSANT_FLAG_POS{12};
constexpr uint8_t PROMOTION_MASK_POS{13};

constexpr uint16_t TO_MASK{0b111111 << TO_SQUARE_POS};
constexpr uint16_t FROM_MASK{0b111111 << FROM_SQUARE_POS};
constexpr uint16_t EN_PASSANT_FLAG{0b1 << EN_PASSANT_FLAG_POS};
constexpr uint16_t PROMOTION_MASK{0b111 << PROMOTION_MASK_POS};

constexpr uint8_t GET_TO_SQUARE(Move move) {
  return ((move & TO_MASK) >> TO_SQUARE_POS);
}

constexpr uint8_t GET_FROM_SQUARE(Move move) {
  return ((move & FROM_MASK) >> FROM_SQUARE_POS);
}

constexpr uint8_t GET_EN_PASSANT_FLAG(Move move) {
  return ((move & EN_PASSANT_FLAG) >> EN_PASSANT_FLAG_POS);
}

constexpr uint8_t GET_PROMOTION_PIECE(Move move) {
  return ((move & PROMOTION_MASK) >> PROMOTION_MASK_POS);
}

constexpr Move ENCODE_MOVE(uint8_t to_square, uint8_t from_square,
                           uint8_t en_passant_flag, uint8_t promotionPiece) {
  return (static_cast<Move>(promotionPiece) << PROMOTION_MASK_POS) |
         (static_cast<Move>(en_passant_flag) << EN_PASSANT_FLAG_POS) |
         (static_cast<Move>(from_square) << FROM_SQUARE_POS) |
         (static_cast<Move>(to_square) << TO_SQUARE_POS);
}