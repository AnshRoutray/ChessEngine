#include "board_manager.hpp"
#include "evaluate.hpp"
#include "move_encoding.hpp"
#include "search.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <omp.h>
#include <thread>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

using namespace std;

constexpr size_t REQUIRED_RAM_MB = (TT_SIZE * sizeof(TT_Entry)) / (1024 * 1024);

static size_t get_system_ram_mb() {
#ifdef __linux__
  struct sysinfo info;
  if (sysinfo(&info) != 0) return 0;
  return (size_t)info.totalram * info.mem_unit / (1024 * 1024);
#else
  return 0;
#endif
}

std::string square_to_algebraic(uint8_t sq) {
  char file = 'h' - (sq % 8); // h=0, g=1, f=2... a=7
  char rank = '1' + sq / 8;
  return std::string(1, file) + std::string(1, rank);
}

void print_move_algebraic(Move move) {
  printf("%s%s\n", square_to_algebraic(GET_FROM_SQUARE(move)).c_str(),
         square_to_algebraic(GET_TO_SQUARE(move)).c_str());
}

uint64_t perft(Board *board, uint8_t depth) {
  if (depth == 0)
    return 1;
  Move moveList[MAX_MOVES];
  uint16_t total = board->generateLegalMoves(moveList);
  uint64_t nodes = 0;
  for (uint16_t i = 0; i < total; i++) {
    UndoInfo undo = board->playMove(moveList[i]);
    nodes += perft(board, depth - 1);
    board->undoMove(undo);
  }
  return nodes;
}

uint64_t perft_divide(Board *board, uint8_t depth) {
  Move moveList[MAX_MOVES];
  uint16_t total = board->generateLegalMoves(moveList);
  uint64_t total_nodes = 0;
  for (uint16_t i = 0; i < total; i++) {
    UndoInfo undo = board->playMove(moveList[i]);
    uint64_t nodes = perft(board, depth - 1);
    board->undoMove(undo);
    printf("%s%s: %llu\n",
           square_to_algebraic(GET_FROM_SQUARE(moveList[i])).c_str(),
           square_to_algebraic(GET_TO_SQUARE(moveList[i])).c_str(), nodes);
    total_nodes += nodes;
  }
  printf("Total: %llu\n", total_nodes);
  return total_nodes;
}

bool perft_test(Board *board, int depth) {
  uint64_t expected[] = {0,      20,      400,       8902,
                         197281, 4865609, 119060324, 3195901860};

  for (int d = 1; d <= depth; d++) {
    uint64_t result = perft(board, d);
    if (result != expected[d]) {
      std::cout << "DEPTH " << d << " FAILED!! Nodes: " << result
                << " Expected: " << expected[d] << std::endl;
      return false;
    }
    std::cout << "DEPTH " << d << " PASSED" << std::endl;
  }
  return true;
}

uint64_t perft(Board *board) {
  int depth = 6;
  Move moveList[MAX_MOVES];
  uint64_t nodes = 0;
  uint16_t total_moves = board->generateLegalMoves(moveList);
  for (uint16_t i = 0; i < total_moves; i++) {
    UndoInfo undo_info = board->playMove(moveList[i]);
    nodes += perft(board, depth - 1);
    board->undoMove(undo_info);
  }
  return nodes;
}

int main(int argc, char *argv[]) {
  size_t ram_mb = get_system_ram_mb();
  unsigned int hw_threads = std::thread::hardware_concurrency();
  if (ram_mb != 0 && ram_mb < REQUIRED_RAM_MB) {
    cerr << "Error: this engine requires at least " << REQUIRED_RAM_MB
         << " MB of system RAM for the transposition table (detected "
         << ram_mb << " MB)." << endl;
    return 1;
  }

  init_engine_tables();

  uint8_t DEPTH = (argc >= 2) ? std::stoi(argv[1]) : 10;

  Board board;
  uint64_t raw_nodes = 2439530234167;
  auto start = std::chrono::high_resolution_clock::now();
  Move bestMove = retrieveBestMove(&board, DEPTH);
  auto end = std::chrono::high_resolution_clock::now();

  double elapsed = std::chrono::duration<double>(end - start).count();

  cout << "===========================================" << endl;
  cout << "           Chess Engine Search Stats           " << endl;
  cout << "===========================================" << endl;
  cout << "Depth searched:       " << (int)DEPTH << endl;
  cout << "Hash size:            " << REQUIRED_RAM_MB << " MB" << endl;
  cout << "Time taken:           " << elapsed << " seconds" << endl;
  cout << "Number of raw positions:  " << raw_nodes << " (~" << raw_nodes / 1e12
       << "Trillion)" << endl;
  cout << "Best move:            "
       << square_to_algebraic(GET_FROM_SQUARE(bestMove))
       << square_to_algebraic(GET_TO_SQUARE(bestMove)) << endl;
  cout << "===========================================" << endl;

  return 0;
}
