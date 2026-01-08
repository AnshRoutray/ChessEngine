#pragma once

#include "move_encoding.hpp"
#include <cstdint>

// Piece bitboard initial positions
constexpr uint64_t WHITE_PAWN_INIT{0b11111111ULL << (8 * 1)};
constexpr uint64_t WHITE_KNIGHT_INIT{0b01000010ULL};
constexpr uint64_t WHITE_BISHOP_INIT{0b00100100ULL};
constexpr uint64_t WHITE_ROOK_INIT{0b10000001ULL};
constexpr uint64_t WHITE_QUEEN_INIT{0b00010000ULL};
constexpr uint64_t WHITE_KING_INIT{0b00001000ULL};

constexpr uint64_t BLACK_PAWN_INIT{0b11111111ULL << (8 * 6)};
constexpr uint64_t BLACK_KNIGHT_INIT{0b01000010ULL << (8 * 7)};
constexpr uint64_t BLACK_BISHOP_INIT{0b00100100ULL << (8 * 7)};
constexpr uint64_t BLACK_ROOK_INIT{0b10000001ULL << (8 * 7)};
constexpr uint64_t BLACK_QUEEN_INIT{0b00010000ULL << (8 * 7)};
constexpr uint64_t BLACK_KING_INIT{0b00001000ULL << (8 * 7)};

// Turn indicators
constexpr uint8_t WHITE_TURN{1};
constexpr uint8_t BLACK_TURN{0};

// Castling rights
constexpr uint8_t CASTLE_SHORT_AND_LONG{0};
constexpr uint8_t CASTLE_SHORT_NO_LONG{1};
constexpr uint8_t CASTLE_LONG_NO_SHORT{2};
constexpr uint8_t CASTLE_NO_SHORT_NO_LONG{3};

constexpr int MAX_MOVES = 256;
constexpr uint8_t FRIEND = 0;
constexpr uint8_t ENEMY = 1;

struct UndoInfo {
  Move previous_previous_move;
  uint8_t previous_castle_state[2];
  uint8_t captured_piece;
};

class Board {
public:
  uint16_t generateLegalMoves(Move moveList[MAX_MOVES]);
  UndoInfo playMove(Move move);
  void undoMove(UndoInfo undo_info);
  bool operator==(Board other_board);
  Board();
  Board(uint64_t pawns, uint64_t knights, uint64_t bishops, uint64_t rooks, 
        uint64_t queen, uint64_t king, uint64_t enemy_pawns, uint64_t enemy_knights, 
        uint64_t enemy_bishops, uint64_t enemy_rooks, uint64_t enemy_queen, 
        uint64_t enemy_king, uint8_t castle_state[2], 
        uint8_t turn, Move previous_move);

private:
  uint64_t pawns;
  uint64_t knights;
  uint64_t bishops;
  uint64_t rooks;
  uint64_t queen;
  uint64_t king;

  uint64_t enemy_pawns;
  uint64_t enemy_knights;
  uint64_t enemy_bishops;
  uint64_t enemy_rooks;
  uint64_t enemy_queen;
  uint64_t enemy_king;

  uint64_t friendly_pieces;
  uint64_t enemy_pieces;

  uint8_t castle_state[2];
  uint8_t turn;
  Move previous_move;
    
  uint8_t piece_locations[64];
  uint64_t* piece_map[7][2];

  inline uint8_t get_first_index(uint64_t bitboard);
  inline uint64_t shift_piece(uint64_t bitboard, int places);
  inline void clear_piece(uint64_t &bitboard, uint8_t index);
  bool is_square_attacked(uint64_t bitboard, uint8_t square);
  void init_piece_locations();
};
