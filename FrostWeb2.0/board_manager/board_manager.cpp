#include "board_manager.hpp"
#include "lookup_tables.hpp"
#include "move_encoding.hpp"
#include <cmath>
#include <cstdint>
#include <immintrin.h>

Board::Board()
    : pawns{WHITE_PAWN_INIT}, knights{WHITE_KNIGHT_INIT},
      bishops{WHITE_BISHOP_INIT}, rooks{WHITE_ROOK_INIT},
      queen{WHITE_QUEEN_INIT}, king{WHITE_KING_INIT},
      enemy_pawns{BLACK_PAWN_INIT}, enemy_knights{BLACK_KNIGHT_INIT},
      enemy_bishops{BLACK_BISHOP_INIT}, enemy_rooks{BLACK_ROOK_INIT},
      enemy_queen{BLACK_QUEEN_INIT}, enemy_king{BLACK_KING_INIT},
      turn{WHITE_TURN} {
  friendly_pieces = pawns | knights | bishops | rooks | queen | king;
  enemy_pieces = enemy_pawns | enemy_knights | enemy_bishops | enemy_queen |
                 enemy_rooks | enemy_king;
  castle_state[0] = CASTLE_SHORT_AND_LONG;
  castle_state[1] = CASTLE_SHORT_AND_LONG;
  init_diagonal_attack_lookup_table();
  init_straight_attack_lookup_table();
  init_piece_locations();
}

Board::Board(uint64_t pawns, uint64_t knights, uint64_t bishops, uint64_t rooks, 
        uint64_t queen, uint64_t king, uint64_t enemy_pawns, uint64_t enemy_knights, 
        uint64_t enemy_bishops, uint64_t enemy_rooks, uint64_t enemy_queen, 
        uint64_t enemy_king, uint8_t castle_state[2], uint8_t turn, 
        Move previous_move): pawns(pawns), knights(knights), 
        bishops(bishops), rooks(rooks), queen(queen),
        king(king), enemy_pawns(enemy_pawns), 
        enemy_knights(enemy_knights), enemy_bishops(enemy_bishops),
        enemy_rooks(enemy_rooks), enemy_queen(enemy_queen),
        enemy_king(enemy_king), castle_state{castle_state[0], castle_state[1]},
        turn(turn), previous_move(previous_move) {
          friendly_pieces = pawns | knights | bishops | rooks | queen | king;
          enemy_pieces = enemy_pawns | enemy_knights | enemy_bishops |
                         enemy_rooks | enemy_queen | enemy_king;
          init_diagonal_attack_lookup_table();
          init_straight_attack_lookup_table();
          init_piece_locations();
        }

void Board::init_piece_locations(){
  piece_map[PAWN_PIECE][0] = &pawns;
  piece_map[KNIGHT_PIECE][0] = &knights;
  piece_map[BISHOP_PIECE][0] = &bishops;
  piece_map[ROOK_PIECE][0] = &rooks;
  piece_map[QUEEN_PIECE][0] = &queen;
  piece_map[KING_PIECE][0] = &king;
  piece_map[PAWN_PIECE][1] = &enemy_pawns;
  piece_map[KNIGHT_PIECE][1] = &enemy_knights;
  piece_map[BISHOP_PIECE][1] = &enemy_bishops;
  piece_map[ROOK_PIECE][1] = &enemy_rooks;
  piece_map[QUEEN_PIECE][1] = &enemy_queen;
  piece_map[KING_PIECE][1] = &enemy_king;
  for(uint8_t i = 0; i < 64; i++){
    piece_locations[i] = EMPTY_PIECE;
  }
  uint64_t bitboard = pawns | enemy_pawns;
  uint8_t index = 0;
  while(bitboard != 0){
      index = __builtin_ctzll(bitboard);
      piece_locations[index] = PAWN_PIECE;
      bitboard &= (bitboard - 1);
  } 
  bitboard = knights | enemy_knights;
  index = 0;
  while(bitboard != 0){
      index = __builtin_ctzll(bitboard);
      piece_locations[index] = KNIGHT_PIECE;
      bitboard &= (bitboard - 1);
  }
  bitboard = bishops | enemy_bishops;
  index = 0;
  while(bitboard != 0){
      index = __builtin_ctzll(bitboard);
      piece_locations[index] = BISHOP_PIECE;
      bitboard &= (bitboard - 1);
  } 
  bitboard = rooks | enemy_rooks;
  index = 0;
  while(bitboard != 0){
      index = __builtin_ctzll(bitboard);
      piece_locations[index] = ROOK_PIECE;
      bitboard &= (bitboard - 1);
  } 
  bitboard = queen | enemy_queen;
  index = 0;
  while(bitboard != 0){
      index = __builtin_ctzll(bitboard);
      piece_locations[index] = QUEEN_PIECE;
      bitboard &= (bitboard - 1);
  } 
  piece_locations[__builtin_ctzll(king)] = KING_PIECE;
  piece_locations[__builtin_ctzll(enemy_king)] = KING_PIECE;
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

bool Board::is_square_attacked(uint64_t bitboard, uint8_t square) {
  uint64_t isAttacked =
      (DIAGONAL_ATTACKS[square][_pext_u64(bitboard, DIAGONALS[square])] &
       (enemy_bishops | enemy_queen));
  isAttacked |= (STRAIGHT_ATTACKS[square][_pext_u64(
                     bitboard, FILE[square % 8] ^ RANK[square / 8])] &
                 (enemy_rooks | enemy_queen));
  isAttacked |= (KNIGHT_ATTACKS[square] & enemy_knights);
  isAttacked |= KING_ATTACKS[square] & enemy_king;
  if (turn ? square < 56 : square > 7) {
    if (square % 8 < 7) {
      isAttacked |= turn ? ((1ULL << (square + 9)) & enemy_pawns)
                         : ((1ULL << (square - 7)) & enemy_pawns);
    }
    if (square % 8 > 0) {
      isAttacked |= turn ? ((1ULL << (square + 7)) & enemy_pawns)
                         : ((1ULL << (square - 9)) & enemy_pawns);
    }
  }
  return isAttacked != 0;
}

UndoInfo Board::playMove(Move move) {
  int8_t to_square = static_cast<int8_t>(GET_TO_SQUARE(move));
  int8_t from_square = static_cast<int8_t>(GET_FROM_SQUARE(move));
  uint8_t en_passant_flag = GET_EN_PASSANT_FLAG(move);
  uint8_t promotion_piece = GET_PROMOTION_PIECE(move);
  uint8_t piece_type = piece_locations[from_square];
  uint64_t *piece = piece_map[piece_type][FRIEND];
  UndoInfo info = {previous_move, {castle_state[0], castle_state[1]},
  EMPTY_PIECE};
  if(enemy_pieces & (1ULL << to_square)){
    info.captured_piece = piece_locations[to_square];
  }
  *piece &= ~(1ULL << from_square);
  piece_locations[from_square] = 0; 
  
  //IF OPPONENT'S ROOK CAPTURED, UPDATE CASTLING FLAGS FOR OPPONENT

  if(piece_locations[to_square] == ROOK_PIECE){
    if(to_square == 0 || to_square == 56){
      if(castle_state[!turn] == CASTLE_SHORT_AND_LONG){
        castle_state[!turn] = CASTLE_LONG_NO_SHORT;
      }
      else if(castle_state[!turn] == CASTLE_SHORT_NO_LONG){
        castle_state[!turn] = CASTLE_NO_SHORT_NO_LONG;
      }
    }
    else if(to_square == 63 || to_square == 7){
      if(castle_state[!turn] == CASTLE_SHORT_AND_LONG){
        castle_state[!turn] = CASTLE_SHORT_NO_LONG;
      }
      else if(castle_state[!turn] == CASTLE_LONG_NO_SHORT){
        castle_state[!turn] = CASTLE_NO_SHORT_NO_LONG;
      }
    }
  }
  if(piece_locations[to_square] != EMPTY_PIECE){
    *piece_map[piece_locations[to_square]][ENEMY] &= ~(1ULL << to_square);
  }
  //EN PASSANT CHECK
  if(en_passant_flag){
    uint8_t en_passant_square = to_square - (turn ? 8 : -8);
    enemy_pawns &= ~(1ULL << en_passant_square);
    piece_locations[en_passant_square] = 0;
  }
  else if(piece_type == KING_PIECE){
    if(abs(to_square - from_square) > 1){
        if(to_square < from_square){
          rooks &= ~(1ULL << (to_square - 1));
          rooks |= 1ULL << (to_square + 1);
          piece_locations[to_square - 1] = 0;
          piece_locations[to_square + 1] = ROOK_PIECE;
        }
        else {
          rooks &= ~(1ULL << (to_square + 1));
          rooks |= 1ULL << (to_square - 1);
          piece_locations[to_square + 1] = 0;
          piece_locations[to_square - 1] = ROOK_PIECE; 
        }
    }
    castle_state[turn] = CASTLE_NO_SHORT_NO_LONG;
  }
  else if(piece_type == ROOK_PIECE){
    if(from_square == 0 || from_square == 56){
      if(castle_state[turn] == CASTLE_SHORT_AND_LONG){
        castle_state[turn] = CASTLE_LONG_NO_SHORT;
      }
      else if(castle_state[turn] == CASTLE_SHORT_NO_LONG){
        castle_state[turn] = CASTLE_NO_SHORT_NO_LONG;
      }
    }
    else if(from_square == 7 || from_square == 63){
      if(castle_state[turn] == CASTLE_SHORT_AND_LONG){
        castle_state[turn] = CASTLE_SHORT_NO_LONG;
      }
      else if(castle_state[turn] == CASTLE_LONG_NO_SHORT){
        castle_state[turn] = CASTLE_NO_SHORT_NO_LONG;
      }
    }
  }
  if(promotion_piece == 0){
    *piece |= 1ULL << to_square;
    piece_locations[to_square] = piece_type;
  }
  else if(promotion_piece == QUEEN_PIECE){
    queen |= 1ULL << to_square;
    piece_locations[to_square] = QUEEN_PIECE;
  }
  else if(promotion_piece == ROOK_PIECE){
    rooks |= 1ULL << to_square;
    piece_locations[to_square] = ROOK_PIECE;
  }
  else if(promotion_piece == KNIGHT_PIECE){
    knights |= 1ULL << to_square;
    piece_locations[to_square] = KNIGHT_PIECE;
  }
  else if(promotion_piece == BISHOP_PIECE){
    bishops |= 1ULL << to_square;
    piece_locations[to_square] = BISHOP_PIECE;
  }
  previous_move = move;
  turn = !turn;
  std::swap(pawns, enemy_pawns);
  std::swap(knights, enemy_knights);
  std::swap(bishops, enemy_bishops);
  std::swap(rooks, enemy_rooks);
  std::swap(queen, enemy_queen);
  std::swap(king, enemy_king);
  std::swap(friendly_pieces, enemy_pieces);
  return info;
}

//Update piece location map

void Board::undoMove(UndoInfo undo_info){
  int8_t prev_to_move = static_cast<int8_t>(GET_TO_SQUARE(previous_move));
  int8_t prev_from_move = static_cast<int8_t>(GET_FROM_SQUARE(previous_move));
  uint8_t prev_en_passant_flag = GET_EN_PASSANT_FLAG(previous_move);
  uint8_t prev_promotion_piece = GET_PROMOTION_PIECE(previous_move);
  uint8_t piece_type = piece_locations[prev_to_move];
  uint64_t *piece = piece_map[piece_type][ENEMY];
  *piece &= ~(1ULL << prev_to_move);
  piece_locations[prev_to_move] = EMPTY_PIECE;
  if(undo_info.captured_piece != 0){
    *piece_map[undo_info.captured_piece][FRIEND] |= (1ULL << prev_to_move);
    piece_locations[prev_to_move] = undo_info.captured_piece;
  }
  if(piece_type == KING_PIECE && abs(prev_to_move - prev_from_move) == 2){
    if(prev_to_move == 1 || prev_to_move == 57){
      enemy_rooks &= ~(1ULL << (prev_to_move + 1));
      piece_locations[prev_to_move + 1] = EMPTY_PIECE;
      enemy_rooks |= (1ULL << (prev_to_move - 1));
      piece_locations[prev_to_move - 1] = ROOK_PIECE;
    }
    else {
      enemy_rooks &= ~(1ULL << (prev_to_move - 1));
      piece_locations[prev_to_move - 1] = EMPTY_PIECE;
      enemy_rooks |= (1ULL << (prev_to_move + 2));
      piece_locations[prev_to_move + 2] = ROOK_PIECE;
    }
  }
  else if(prev_en_passant_flag){
    uint8_t square = (prev_to_move + (turn ? 8 : -8));
    pawns |= 1ULL << square;
    piece_locations[square] = PAWN_PIECE;
  }
  if(prev_promotion_piece == 0){
    *piece |= (1ULL << prev_from_move);
    piece_locations[prev_from_move] = piece_type;
  }
  else{
    enemy_pawns |= (1ULL << prev_from_move);
    piece_locations[prev_from_move] = PAWN_PIECE;
  } 
  castle_state[0] = undo_info.previous_castle_state[0];
  castle_state[1] = undo_info.previous_castle_state[1];
  previous_move = undo_info.previous_previous_move;
  turn = !turn;
  std::swap(pawns, enemy_pawns);
  std::swap(knights, enemy_knights);
  std::swap(bishops, enemy_bishops);
  std::swap(rooks, enemy_rooks);
  std::swap(queen, enemy_queen);
  std::swap(king, enemy_king);
  std::swap(friendly_pieces, enemy_pieces);
}

uint16_t Board::generateLegalMoves(Move moveList[MAX_MOVES]) {

  uint16_t moveIndex = 0;

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
  uint64_t last_piece_position = 1ULL << previous_move_to;
  if ((last_piece_position & enemy_pawns) &&
      (abs(previous_move_to - previous_move_from) == 16)) {
    if (((last_piece_position & ~FILE[7]) << 1) & pawns) {
      moveList[moveIndex++] = ENCODE_MOVE(
          previous_move_to + (turn ? 8 : -8), previous_move_to + 1, 1, 0);
    }
    if (((last_piece_position & ~FILE[0]) >> 1) & pawns) {
      moveList[moveIndex++] = ENCODE_MOVE(
          previous_move_to + (turn ? 8 : -8), previous_move_to - 1, 1, 0);
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

  // Generating Diagonal Moves
  uint64_t diagonal_pieces = queen | bishops;
  while (diagonal_pieces) {
    index = __builtin_ctzll(diagonal_pieces);
    uint16_t key = _pext_u64(friendly_pieces | enemy_pieces, DIAGONALS[index]);
    uint64_t attack_bitboard = DIAGONAL_ATTACKS[index][key];
    attack_bitboard &= ~friendly_pieces;
    while (attack_bitboard) {
      uint8_t temp_index = __builtin_ctzll(attack_bitboard);
      moveList[moveIndex++] = ENCODE_MOVE(temp_index, index, 0, 0);
      attack_bitboard &= attack_bitboard - 1;
    }
    diagonal_pieces &= diagonal_pieces - 1;
  }

  // Generating Straight Moves
  uint64_t straight_pieces = queen | rooks;
  while (straight_pieces) {
    index = __builtin_ctzll(straight_pieces);
    uint16_t key = _pext_u64(friendly_pieces | enemy_pieces,
                             FILE[index % 8] ^ RANK[index / 8]);
    uint64_t attack_bitboard = STRAIGHT_ATTACKS[index][key];
    attack_bitboard &= ~friendly_pieces;
    while (attack_bitboard) {
      uint8_t temp_index = __builtin_ctzll(attack_bitboard);
      moveList[moveIndex++] = ENCODE_MOVE(temp_index, index, 0, 0);
      attack_bitboard &= attack_bitboard - 1;
    }
    straight_pieces &= straight_pieces - 1;
  }

  // Generating King Moves
  uint8_t king_square = __builtin_ctzll(king);
  uint64_t king_attacks = KING_ATTACKS[king_square] & ~friendly_pieces;
  while (king_attacks) {
    index = __builtin_ctzll(king_attacks);
    moveList[moveIndex++] = ENCODE_MOVE(index, king_square, 0, 0);
    king_attacks &= king_attacks - 1;
  }

  // Castling
  uint64_t board_layout = friendly_pieces | enemy_pieces;
  if (castle_state[turn] == CASTLE_SHORT_AND_LONG ||
      castle_state[turn] == CASTLE_SHORT_NO_LONG) {
    if (!(board_layout & (1ULL << (king_square - 1))) &&
        !(board_layout & (1ULL << (king_square - 2))) &&
        !(is_square_attacked(board_layout, king_square - 1) ||
          is_square_attacked(board_layout, king_square - 2) ||
          is_square_attacked(board_layout, king_square))) {
      moveList[moveIndex++] = ENCODE_MOVE(king_square - 2, king_square, 0, 0);
    }
  }
  if (castle_state[turn] == CASTLE_SHORT_AND_LONG ||
      castle_state[turn] == CASTLE_LONG_NO_SHORT) {
    if (!(board_layout & (1ULL << (king_square + 1))) &&
        !(board_layout & (1ULL << (king_square + 2))) &&
        !(is_square_attacked(board_layout, king_square + 1) ||
          is_square_attacked(board_layout, king_square + 2) ||
          is_square_attacked(board_layout, king_square))) {
      moveList[moveIndex++] = ENCODE_MOVE(king_square + 2, king_square, 0, 0);
    }
  }

  uint16_t final_index = 0;
  for (int i = 0; i < moveIndex; ++i) {
    uint8_t to_square = GET_TO_SQUARE(moveList[i]);
    uint8_t from_square = GET_FROM_SQUARE(moveList[i]);
    uint8_t en_passant_flag = GET_EN_PASSANT_FLAG(moveList[i]);
    uint64_t bitboard = friendly_pieces | enemy_pieces;
    if (king & (1ULL << from_square)) {
      king_square = to_square;
    }
    bitboard &= ~(1ULL << from_square);
    bitboard |= (1ULL << to_square);
    if (en_passant_flag) {
      bitboard &= ~(1ULL << (to_square + (turn ? -8 : 8)));
    }
    uint64_t isAttacked = is_square_attacked(bitboard, king_square);
    if (!isAttacked) {
      moveList[final_index] = moveList[i];
      final_index++;
    }
  }
  return final_index;
}

bool Board::operator==(Board other_board){
  bool pawns_equal = (pawns == other_board.pawns);
  bool enemy_pawns_equal = (enemy_pawns == other_board.enemy_pawns);
  bool knights_equal = (knights == other_board.knights);
  bool enemy_knights_equal = (enemy_knights == other_board.enemy_knights);
  bool bishops_equal = (bishops == other_board.bishops);
  bool enemy_bishops_equal = (enemy_bishops == other_board.enemy_bishops);
  bool rooks_equal = (rooks == other_board.rooks);
  bool enemy_rooks_equal = (enemy_rooks == other_board.enemy_rooks);
  bool queens_equal = (queen == other_board.queen);
  bool enemy_queens_equal = (enemy_queen == other_board.enemy_queen);
  bool king_equal = (king == other_board.king);
  bool enemy_king_equal = (enemy_king == other_board.enemy_king);
  bool pieces_equal = pawns_equal && enemy_pawns_equal && knights_equal &&
      enemy_knights_equal && bishops_equal && enemy_bishops_equal &&
      rooks_equal && enemy_rooks_equal &&queens_equal &&
      enemy_queens_equal && king_equal && enemy_king_equal;
  bool friendly_pieces_equal = friendly_pieces == other_board.friendly_pieces;
  bool enemy_pieces_equal = enemy_pieces == other_board.enemy_pieces;
  bool turn_equal = turn == other_board.turn;
  bool previous_move_equal = previous_move == other_board.previous_move;

  bool piece_location_map_equal = true;
  for(int i = 0; i < 64; i++){
      if(piece_locations[i] != other_board.piece_locations[i]){
        piece_location_map_equal = false;
        break;
      }
  }
  bool castling_states_equal = (castle_state[0] == other_board.castle_state[0])
      && (castle_state[1] == other_board.castle_state[1]);

  bool result = pieces_equal && friendly_pieces_equal && enemy_pieces_equal &&
      turn_equal && previous_move_equal && piece_location_map_equal &&
      castling_states_equal;
  return result;
}
