#include "move_encoding.hpp"
#include <cmath>
#include <cstdint>
#include "lookup_tables.hpp"
#include <immintrin.h>

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

class Board {
public:
  uint16_t generateLegalMoves(Move *moveList);
  void playMove(Move move);

private:
  uint64_t white_pawns;
  uint64_t white_knights;
  uint64_t white_bishops;
  uint64_t white_rooks;
  uint64_t white_queen;
  uint64_t white_king;

  uint64_t black_pawns;
  uint64_t black_knights;
  uint64_t black_bishops;
  uint64_t black_rooks;
  uint64_t black_queen;
  uint64_t black_king;
  uint8_t white_castle_state;
  uint8_t black_castle_state;
  uint8_t turn;
  Move previous_move;

  inline uint8_t get_first_index(uint64_t bitboard);
  inline uint64_t shift_piece(uint64_t bitboard, int places);
  inline void clear_piece(uint64_t &bitboard, uint8_t index);
};

Board::Board()
    : white_pawns{WHITE_PAWN_INIT}, white_knights{WHITE_KNIGHT_INIT},
      white_bishops{WHITE_BISHOP_INIT}, white_rooks{WHITE_ROOK_INIT},
      white_queen{WHITE_QUEEN_INIT}, white_king{WHITE_KING_INIT},
      black_pawns{BLACK_PAWN_INIT}, black_knights{BLACK_KNIGHT_INIT},
      black_bishops{BLACK_BISHOP_INIT}, black_rooks{BLACK_ROOK_INIT},
      black_queen{BLACK_QUEEN_INIT}, black_king{BLACK_KING_INIT},
      turn{WHITE_TURN}, white_castle_state{CASTLE_SHORT_AND_LONG},
      black_castle_state{CASTLE_SHORT_AND_LONG} {
        init_diagonal_attack_lookup_table();
      }

inline void Board::clear_piece(uint64_t &bitboard, uint8_t index) {
  bitboard &= ~(1ULL << (turn ? index : 63 - index));
}

inline uint8_t Board::get_first_index(uint64_t bitboard) {
  if (bitboard == 0) {
    return 0;
  }
  if (turn) {
    return __builtin_ctzll(bitboard);
  } else {
    return __builtin_clzll(bitboard);
  }
}

inline uint64_t Board::shift_piece(uint64_t bitboard, int places) {
  if (turn) {
    bitboard <<= places;
  } else {
    bitboard >>= places;
  }
  return bitboard;
}

void Board::playMove(Move move) {}

uint16_t Board::generateLegalMoves(Move moveList[MAX_MOVES]) {

  uint64_t pawns = turn ? white_pawns : black_pawns;
  uint64_t knights = turn ? white_knights : black_knights;
  uint64_t bishops = turn ? white_bishops : black_bishops;
  uint64_t rooks = turn ? white_rooks : black_rooks;
  uint64_t queen = turn ? white_queen : black_queen;
  uint64_t king = turn ? white_king : black_king;

  uint64_t enemy_pawns = turn ? black_pawns : white_pawns;
  uint64_t enemy_knights = turn ? black_knights : white_knights;
  uint64_t enemy_bishops = turn ? black_bishops : white_bishops;
  uint64_t enemy_rooks = turn ? black_rooks : white_rooks;
  uint64_t enemy_queen = turn ? black_queen : white_queen;
  uint64_t enemy_king = turn ? black_king : white_king;

  uint16_t moveIndex = 0;

  const uint64_t friendly_pieces =
      pawns | knights | bishops | rooks | queen | king;
  const uint64_t enemy_pieces = enemy_pawns | enemy_knights | enemy_bishops |
                                enemy_queen | enemy_rooks | enemy_king;

  // Generating pawn moves
  uint64_t single_pawn_moves =
      (shift_piece(pawns, 8)) & ~friendly_pieces & ~enemy_pieces;
  uint64_t double_pawn_moves =
      (shift_piece(single_pawn_moves, 8)) & ~friendly_pieces & ~enemy_pieces;

  uint8_t index = get_first_index(single_pawn_moves);
  while (single_pawn_moves) {
    if (index < 56) {
      moveList[moveIndex++] =
          turn ? (ENCODE_MOVE(index, index - 8, 0, 0))
               : (ENCODE_MOVE(63 - index, 63 - (index - 8), 0, 0));
    } else {
      for (int i = 1; i <= 4; i++) {
        moveList[moveIndex++] =
            turn ? ENCODE_MOVE(index, index - 8, 0, i)
                 : ENCODE_MOVE(63 - index, 63 - (index - 8), 0, i);
      }
    }
    clear_piece(single_pawn_moves, index);
    index = get_first_index(single_pawn_moves);
  }
  index = get_first_index(double_pawn_moves);
  while (double_pawn_moves && index < 40) {
    moveList[moveIndex++] =
        turn ? ENCODE_MOVE(index, index - 16, 0, 0)
             : ENCODE_MOVE(63 - index, 63 - (index - 16), 0, 0);
    clear_piece(double_pawn_moves, index);
    index = get_first_index(double_pawn_moves);
  }

  uint64_t diagonal_pawn_attacks_left = 0;
  uint64_t diagonal_pawn_attacks_right = 0;
  if (turn) {
    diagonal_pawn_attacks_left = ((pawns & ~FILE[0]) << 9) & enemy_pieces;
    diagonal_pawn_attacks_right = ((pawns & ~FILE[7]) << 7) & enemy_pieces;
  } else {
    diagonal_pawn_attacks_left = ((pawns & ~FILE[7]) >> 9) & enemy_pieces;
    diagonal_pawn_attacks_right = ((pawns & ~FILE[0]) >> 7) & enemy_pieces;
  }
  index = get_first_index(diagonal_pawn_attacks_left);
  while (diagonal_pawn_attacks_left) {
    if (index < 56) {
      moveList[moveIndex++] =
          turn ? ENCODE_MOVE(index, index - 9, 0, 0)
               : ENCODE_MOVE(63 - index, 63 - (index - 9), 0, 0);
    } else {
      for (int i = 1; i <= 4; i++) {
        moveList[moveIndex++] =
            turn ? ENCODE_MOVE(index, index - 9, 0, i)
                 : ENCODE_MOVE(63 - index, 63 - (index - 9), 0, i);
      }
    }
    clear_piece(diagonal_pawn_attacks_left, index);
    index = get_first_index(diagonal_pawn_attacks_left);
  }
  index = get_first_index(diagonal_pawn_attacks_right);
  while (diagonal_pawn_attacks_right) {
    if (index < 56) {
      moveList[moveIndex++] =
          turn ? ENCODE_MOVE(index, index - 7, 0, 0)
               : ENCODE_MOVE(63 - index, 63 - (index - 7), 0, 0);
    } else {
      for (int i = 1; i <= 4; i++) {
        moveList[moveIndex++] =
            turn ? ENCODE_MOVE(index, index - 7, 0, i)
                 : ENCODE_MOVE(63 - index, 63 - (index - 7), 0, i);
      }
    }
    clear_piece(diagonal_pawn_attacks_right, index);
    index = get_first_index(diagonal_pawn_attacks_right);
  }
  int previous_move_to = static_cast<int>(GET_TO_SQUARE(previous_move));
  int previous_move_from = static_cast<int>(GET_FROM_SQUARE(previous_move));
  uint64_t last_piece_position = 1 << previous_move_to;
  if ((last_piece_position & enemy_pawns) &&
      (abs(previous_move_to - previous_move_from) == 16)) {
    if (((last_piece_position & ~FILE[0]) << 1) & pawns) {
      moveList[moveIndex++] = ENCODE_MOVE(
          previous_move_to + 1, previous_move_to + (turn ? 8 : -8), 1, 0);
    }
    if (((last_piece_position & ~FILE[7]) >> 1) & pawns) {
      moveList[moveIndex++] = ENCODE_MOVE(
          previous_move_to - 1, previous_move_to + (turn ? 8 : -8), 1, 0);
    }
  }

  // Generating Knight Moves, might need to fix turn?
  uint64_t temp_knights = knights;
  index = __builtin_ctzll(temp_knights);
  uint64_t knight_attacks = 0;
  uint8_t attack_index = 0;
  while (temp_knights) {
    knight_attacks = KNIGHT_ATTACKS[index] & ~friendly_pieces;
    attack_index = __builtin_ctzll(knight_attacks);
    while (knight_attacks) {
      moveList[moveIndex++] = ENCODE_MOVE(attack_index, index, 0, 0);
      knight_attacks &= knight_attacks - 1;
      attack_index = __builtin_ctzll(knight_attacks);
    }
    temp_knights &= temp_knights - 1;
    index = __builtin_ctzll(temp_knights);
  }

  //Generating Diagonal Moves
  uint64_t diagonal_pieces = queen | bishops;
  while(diagonal_pieces){
    index = __builtin_ctzll(diagonal_pieces);
    uint16_t key = _pext_u64(friendly_pieces | enemy_pieces, DIAGONALS[index]);
    uint64_t attack_bitboard = DIAGONAL_ATTACKS[index][key];
    attack_bitboard &= ~friendly_pieces;
    while(attack_bitboard){
      uint8_t temp_index = __builtin_ctzll(attack_bitboard);
      moveList[moveIndex++] = ENCODE_MOVE(temp_index, index, 0, 0);
      attack_bitboard &= attack_bitboard - 1;
    }
    diagonal_pieces &= diagonal_pieces - 1;
  }

  //Generating Straight Moves
  uint64_t straight_pieces = queen | rooks;
    while(straight_pieces){
    index = __builtin_ctzll(straight_pieces);
    uint16_t key = _pext_u64(friendly_pieces | enemy_pieces, FILE[7 - (index % 8)] ^ RANK[index / 8]);
    uint64_t attack_bitboard = STRAIGHT_ATTACKS[index][key];
    attack_bitboard &= ~friendly_pieces;
    while(attack_bitboard){
      uint8_t temp_index = __builtin_ctzll(attack_bitboard);
      moveList[moveIndex++] = ENCODE_MOVE(temp_index, index, 0, 0);
      attack_bitboard &= attack_bitboard - 1;
    }
    straight_pieces &= straight_pieces - 1;
  }
  
  //Generating King Moves
  uint8_t king_square = __builtin_ctzll(king);
  uint64_t king_attacks = KING_ATTACKS[king_square] & ~friendly_pieces;
  while(king_attacks){
    index = __builtin_ctzll(king_attacks);
    moveList[moveIndex++] = (index, king_square, 0, 0);
    king_attacks &= king_attacks - 1;
  }

  //Filtering out illegal moves NEEDS REVIEW (TERRIBLE CODE)
  uint16_t final_index = 0;
  for(int i = 0; i < moveIndex; ++i){
    uint8_t to_square = GET_TO_SQUARE(moveList[i]);
    uint8_t from_square = GET_FROM_SQUARE(moveList[i]);
    uint8_t en_passant_flag = GET_EN_PASSANT_FLAG(moveList[moveIndex]);
    uint64_t bitboard = friendly_pieces | enemy_pieces;
    if(1 << from_square & king){
      king_square = to_square;
    }
    bitboard &= ~(1 << from_square);
    bitboard |= (1 << to_square);
    if(en_passant_flag){
      bitboard &= ~(1 << (to_square + (turn ? -8 : 8)));
    }
    uint64_t isAttacked = (DIAGONAL_ATTACKS[king_square][_pext_u64(bitboard, DIAGONALS[king_square])] & 
      (enemy_bishops | enemy_queen)) && 1;
    isAttacked |= (STRAIGHT_ATTACKS[king_square][_pext_u64(friendly_pieces | enemy_pieces, 
      FILE[7 - (king_square % 8)] ^ RANK[king_square / 8])] & (enemy_rooks | enemy_queen)); 
    isAttacked |= (KNIGHT_ATTACKS[king_square] & enemy_knights);
    if(turn ? king_square < 56 : king_square > 7){
      if(king_square % 8 < 7){
        isAttacked |= turn ? ((king << 9) & enemy_pawns) : ((king >> 7) & enemy_pawns);
      }
      if(king_square % 8 > 0){
        isAttacked |= turn ? ((king << 7) & enemy_pawns) : ((king >> 9) & enemy_pawns);
      }
    }
    if(!isAttacked){
      moveList[final_index] = moveList[i];
      final_index++;
    }
  }

  return final_index;
}