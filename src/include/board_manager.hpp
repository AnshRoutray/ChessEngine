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

constexpr uint16_t MAX_MOVES = 256;
constexpr uint8_t FRIEND = 0;
constexpr uint8_t ENEMY = 1;

// Tapered-eval score: one score per game phase, blended at eval time.
// Indices 0/1/2 correspond to opening / middlegame / endgame respectively.
struct PhaseScore {
  int16_t op;
  int16_t mg;
  int16_t eg;
  PhaseScore &operator+=(const PhaseScore &o) {
    op += o.op;
    mg += o.mg;
    eg += o.eg;
    return *this;
  }
  PhaseScore &operator-=(const PhaseScore &o) {
    op -= o.op;
    mg -= o.mg;
    eg -= o.eg;
    return *this;
  }
};

struct UndoInfo {
  Move previous_previous_move;
  uint8_t previous_castle_state[2];
  uint8_t captured_piece;
  uint64_t previous_zobrist_hash;
  PhaseScore previous_pst_score_white;
};

extern uint64_t ZOBRIST_VALUES[2][7][64];
extern uint64_t ZOBRIST_TURN;
extern uint64_t ZOBRIST_CASTLE[2][4];
extern uint64_t ZOBRIST_EP_FILE[8];

void init_zobrist_hashes();
void init_engine_tables();

class Board {
public:
  uint16_t generateLegalMoves(Move moveList[MAX_MOVES]);
  uint16_t generateCaptureMoves(Move moveList[MAX_MOVES]);
  void printBoard();
  UndoInfo playMove(Move move);
  bool is_square_attacked(uint64_t bitboard, uint8_t square);
  bool is_square_attacked() {
    return is_square_attacked(friendly_pieces | enemy_pieces,
                              __builtin_ctzll(king));
  }
  void undoMove(UndoInfo undo_info);
  UndoInfo playNullMove();
  void undoNullMove(UndoInfo undo_info);
  bool operator==(Board other_board);
  Board();
  Board(uint64_t pawns, uint64_t knights, uint64_t bishops, uint64_t rooks,
        uint64_t queen, uint64_t king, uint64_t enemy_pawns,
        uint64_t enemy_knights, uint64_t enemy_bishops, uint64_t enemy_rooks,
        uint64_t enemy_queen, uint64_t enemy_king, uint8_t castle_state[2],
        uint8_t turn, Move previous_move);

  Board(const Board &other);
  Board &operator=(const Board &other);

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
  uint64_t *piece_map[7][2];

  uint64_t zobrist_hash;
  PhaseScore pst_score_white;

  inline uint8_t get_first_index(uint64_t bitboard);
  inline uint64_t shift_piece(uint64_t bitboard, int places);
  inline void clear_piece(uint64_t &bitboard, uint8_t index);
  void init_piece_locations_and_hash();
  uint64_t compute_pinned_pieces(uint8_t king_sq);
};
