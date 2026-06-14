#include "board_manager.hpp"
#include "move_encoding.hpp"
#include "search.hpp"
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

constexpr std::size_t REQUIRED_RAM_MB =
    (TT_SIZE * sizeof(TT_Entry)) / (1024 * 1024);

constexpr const char *ENGINE_NAME = "ChessEngine";
constexpr const char *ENGINE_AUTHOR = "Ansh Routray";
constexpr uint8_t DEFAULT_GO_DEPTH = 10;

static std::size_t get_system_ram_mb() {
#ifdef __linux__
  struct sysinfo info;
  if (sysinfo(&info) != 0)
    return 0;
  return (std::size_t)info.totalram * info.mem_unit / (1024 * 1024);
#else
  return 0;
#endif
}

// Square encoding in this engine: idx = rank_idx * 8 + ('h' - file_letter).
// h1 = 0, e1 = 3, a1 = 7, e8 = 59, a8 = 63. Always absolute, never flipped
// by side-to-move.
static uint8_t algebraic_to_square(char file, char rank) {
  return (uint8_t)((rank - '1') * 8 + ('h' - file));
}

static std::string square_to_algebraic(uint8_t sq) {
  char file = (char)('h' - (sq % 8));
  char rank = (char)('1' + sq / 8);
  return std::string{file, rank};
}

static std::string move_to_uci(Move move) {
  std::string out = square_to_algebraic(GET_FROM_SQUARE(move)) +
                    square_to_algebraic(GET_TO_SQUARE(move));
  // Encoded promotion: 1=N, 2=B, 3=R, 4=Q (see move_encoding.hpp).
  switch (GET_PROMOTION_PIECE(move)) {
  case 1: out += 'n'; break;
  case 2: out += 'b'; break;
  case 3: out += 'r'; break;
  case 4: out += 'q'; break;
  default: break;
  }
  return out;
}

// Find the legal move that matches the given UCI string, or 0 if none.
// Matching against the legal move list naturally handles castling, en passant,
// and promotion piece disambiguation without our caller needing to reproduce
// movegen quirks.
static Move parse_uci_move(Board &board, const std::string &uci) {
  if (uci.size() < 4)
    return 0;
  uint8_t from_sq = algebraic_to_square(uci[0], uci[1]);
  uint8_t to_sq = algebraic_to_square(uci[2], uci[3]);
  uint8_t promo = 0;
  if (uci.size() >= 5) {
    switch (uci[4]) {
    case 'n': promo = 1; break;
    case 'b': promo = 2; break;
    case 'r': promo = 3; break;
    case 'q': promo = 4; break;
    }
  }
  Move list[MAX_MOVES];
  uint16_t n = board.generateLegalMoves(list);
  for (uint16_t i = 0; i < n; i++) {
    if (GET_FROM_SQUARE(list[i]) == from_sq &&
        GET_TO_SQUARE(list[i]) == to_sq &&
        GET_PROMOTION_PIECE(list[i]) == promo)
      return list[i];
  }
  return 0;
}

// Convert castle-rights flags to the engine's packed encoding.
static uint8_t pack_castle(bool king_side, bool queen_side) {
  if (king_side && queen_side)
    return CASTLE_SHORT_AND_LONG;
  if (king_side)
    return CASTLE_SHORT_NO_LONG;
  if (queen_side)
    return CASTLE_LONG_NO_SHORT;
  return CASTLE_NO_SHORT_NO_LONG;
}

// Minimal FEN parser. Reads piece placement, side to move, and castling
// rights. En passant target is ignored (engine derives EP from previous_move,
// which we cannot synthesize from FEN alone); halfmove/fullmove counters are
// also ignored. Returns startpos on parse failure.
static Board board_from_fen(const std::string &fen) {
  uint64_t bb[2][7] = {{0}};
  std::size_t i = 0;
  int rank = 7;
  int file = 0;

  while (i < fen.size() && fen[i] != ' ') {
    char c = fen[i++];
    if (c == '/') {
      rank--;
      file = 0;
      continue;
    }
    if (c >= '1' && c <= '8') {
      file += c - '0';
      continue;
    }
    int color = (c >= 'A' && c <= 'Z') ? 0 : 1;
    int piece = 0;
    switch ((char)std::tolower((unsigned char)c)) {
    case 'p': piece = PAWN_PIECE; break;
    case 'n': piece = KNIGHT_PIECE; break;
    case 'b': piece = BISHOP_PIECE; break;
    case 'r': piece = ROOK_PIECE; break;
    case 'q': piece = QUEEN_PIECE; break;
    case 'k': piece = KING_PIECE; break;
    default: return Board();
    }
    if (rank < 0 || rank > 7 || file < 0 || file > 7)
      return Board();
    uint8_t sq = (uint8_t)(rank * 8 + (7 - file));
    bb[color][piece] |= 1ULL << sq;
    file++;
  }
  while (i < fen.size() && fen[i] == ' ')
    i++;

  uint8_t turn = WHITE_TURN;
  if (i < fen.size()) {
    turn = (fen[i] == 'b') ? BLACK_TURN : WHITE_TURN;
    i++;
  }
  while (i < fen.size() && fen[i] == ' ')
    i++;

  bool wk = false, wq = false, bk = false, bq = false;
  while (i < fen.size() && fen[i] != ' ') {
    switch (fen[i]) {
    case 'K': wk = true; break;
    case 'Q': wq = true; break;
    case 'k': bk = true; break;
    case 'q': bq = true; break;
    }
    i++;
  }

  // castle_state is indexed by absolute color: index WHITE_TURN=1 -> white,
  // index BLACK_TURN=0 -> black.
  uint8_t castle_state[2];
  castle_state[WHITE_TURN] = pack_castle(wk, wq);
  castle_state[BLACK_TURN] = pack_castle(bk, bq);

  // The Board constructor takes friendly/enemy bitboards relative to the side
  // to move, not absolute white/black. Pick accordingly.
  int us = (turn == WHITE_TURN) ? 0 : 1;
  int them = 1 - us;
  return Board(bb[us][PAWN_PIECE], bb[us][KNIGHT_PIECE], bb[us][BISHOP_PIECE],
               bb[us][ROOK_PIECE], bb[us][QUEEN_PIECE], bb[us][KING_PIECE],
               bb[them][PAWN_PIECE], bb[them][KNIGHT_PIECE],
               bb[them][BISHOP_PIECE], bb[them][ROOK_PIECE],
               bb[them][QUEEN_PIECE], bb[them][KING_PIECE], castle_state, turn,
               0);
}

static void handle_position(Board &board, std::istringstream &iss) {
  std::string token;
  if (!(iss >> token))
    return;

  if (token == "startpos") {
    board = Board();
    if (!(iss >> token))
      return;
  } else if (token == "fen") {
    std::string fen;
    while (iss >> token && token != "moves") {
      if (!fen.empty())
        fen += ' ';
      fen += token;
    }
    board = board_from_fen(fen);
    if (token != "moves")
      return;
  } else {
    return;
  }

  if (token != "moves")
    return;
  std::string uci_move;
  while (iss >> uci_move) {
    Move m = parse_uci_move(board, uci_move);
    if (m == 0) {
      std::cout << "info string illegal move " << uci_move << '\n'
                << std::flush;
      return;
    }
    board.playMove(m);
  }
}

static void handle_go(Board &board, std::istringstream &iss) {
  int depth = DEFAULT_GO_DEPTH;
  std::string token;
  while (iss >> token) {
    if (token == "depth") {
      iss >> depth;
    }
    // movetime / wtime / btime / infinite are not yet honored; depth-only.
  }
  if (depth < 1)
    depth = 1;
  if (depth > MAX_DEPTH)
    depth = MAX_DEPTH;

  auto start = std::chrono::steady_clock::now();
  Move best = retrieveBestMove(&board, (uint8_t)depth);
  auto end = std::chrono::steady_clock::now();
  long long ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "info depth " << depth << " time " << ms << '\n';
  if (best == 0) {
    std::cout << "bestmove 0000\n" << std::flush;
  } else {
    std::cout << "bestmove " << move_to_uci(best) << '\n' << std::flush;
  }
}

static void uci_loop() {
  Board board;
  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream iss(line);
    std::string cmd;
    if (!(iss >> cmd))
      continue;

    if (cmd == "uci") {
      std::cout << "id name " << ENGINE_NAME << '\n'
                << "id author " << ENGINE_AUTHOR << '\n'
                << "uciok\n"
                << std::flush;
    } else if (cmd == "isready") {
      std::cout << "readyok\n" << std::flush;
    } else if (cmd == "ucinewgame") {
      board = Board();
    } else if (cmd == "position") {
      handle_position(board, iss);
    } else if (cmd == "go") {
      handle_go(board, iss);
    } else if (cmd == "quit") {
      break;
    }
    // stop / setoption / ponderhit / register: silently ignored.
  }
}

int main() {
  std::size_t ram_mb = get_system_ram_mb();
  if (ram_mb != 0 && ram_mb < REQUIRED_RAM_MB) {
    std::cerr << "Error: this engine requires at least " << REQUIRED_RAM_MB
              << " MB of system RAM for the transposition table (detected "
              << ram_mb << " MB)." << std::endl;
    return 1;
  }

  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  init_engine_tables();
  uci_loop();
  return 0;
}
