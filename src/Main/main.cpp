#include "board_manager.hpp"
#include "move_encoding.hpp"
#include <chrono>
#include <iostream>

using namespace std;

int main() {
  Board board = Board();
  Move moveList[MAX_MOVES];
  auto start = std::chrono::high_resolution_clock::now();
  board.generateLegalMoves(moveList);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  cout << duration.count() << "ns" << endl;
}
