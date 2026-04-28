#include "board_manager.hpp"
#include "evaluate.hpp"
#include "move_encoding.hpp"
#include "search.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace std;

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

int main() {
  Board board;
  Move bestMove = retrieveBestMove(&board, 7);
  cout << (int)GET_FROM_SQUARE(bestMove) << endl;
  cout << (int)GET_TO_SQUARE(bestMove) << endl;
  return 0;
}
