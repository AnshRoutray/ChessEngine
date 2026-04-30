#include "search.hpp"
#include "board_manager.hpp"
#include "evaluate.hpp"
#include "move_encoding.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>

TT_Entry TT_TABLE[TT_SIZE];

Move retrieveBestMove(Board *board, uint8_t max_depth) {
  Move best_move = 0;
  for (int depth = 1; depth <= max_depth; depth++) {
    best_move = retrieveBestMoveAtDepth(board, depth);
  }
  return best_move;
}

Move retrieveBestMoveAtDepth(Board *board, uint8_t depth) {
  Move moveList[MAX_MOVES];
  uint16_t total_moves = board->generateLegalMoves(moveList);
  if (total_moves == 0) {
    if (board->is_square_attacked()) {
      return 0xFFFF;
    } else {
      return 0;
    }
  }
  int16_t max_score = -INF;
  uint16_t best_move = 0;
  for (uint16_t move = 0; move < total_moves; ++move) {
    UndoInfo undo_info = board->playMove(moveList[move]);
    int16_t score = (depth == 0) ? -stableSearch(board, -INF, INF)
                                 : -searchBestMove(board, depth - 1, -INF, INF);
    board->undoMove(undo_info);
    if (score > max_score) {
      max_score = score;
      best_move = move;
    }
  }
  return moveList[best_move];
}

int16_t searchBestMove(Board *board, uint8_t depth, int16_t alpha,
                       int16_t beta) {
  uint64_t zobrist_hash = board->zobrist_hash;
  TT_Entry tt_value = TT_TABLE[zobrist_hash % TT_SIZE];
  bool use_TT_best_move = false;
  if (tt_value.zobrist_hash == zobrist_hash) {
    if (tt_value.depth >= depth) {
      if (tt_value.cutoff_flag == EXACT) {
        return tt_value.score;
      } else if (tt_value.cutoff_flag == AT_LEAST) { // cutoff flag is AT LEAST
        if (tt_value.score >= beta) {
          return beta;
        }
      } else {
        if (tt_value.score <= alpha) {
          return alpha;
        }
      }
    }
    use_TT_best_move = true;
  }
  Move moveList[MAX_MOVES];
  uint16_t total_moves = board->generateLegalMoves(moveList);
  if (total_moves == 0) {
    if (board->is_square_attacked()) {
      return -(30000 - depth);
    } else {
      return 0;
    }
  }
  int16_t original_alpha = alpha;
  std::sort(moveList, moveList + total_moves,
            [board, tt_value, use_TT_best_move](Move a, Move b) {
              if (use_TT_best_move &&
                  (a == tt_value.best_move || b == tt_value.best_move)) {
                return a == tt_value.best_move;
              }
              return mvv_lva_heuristic(board, a) > mvv_lva_heuristic(board, b);
            });
  Move best_move = moveList[0];
  int16_t current_score = -INF;
  for (uint16_t move = 0; move < total_moves; ++move) {
    UndoInfo undo_info = board->playMove(moveList[move]);
    int16_t score = (depth == 0)
                        ? -stableSearch(board, -beta, -alpha)
                        : -searchBestMove(board, depth - 1, -beta, -alpha);
    board->undoMove(undo_info);
    if (score >= beta) {
      TT_Entry entry = {depth, AT_LEAST, moveList[move], beta, zobrist_hash};
      TT_TABLE[zobrist_hash % TT_SIZE] = entry;
      return beta;
    }
    if (score > current_score) {
      best_move = moveList[move];
      current_score = score;
    }
    alpha = std::max<int16_t>(alpha,
                              score); // Compare performance with if statement
  }
  TT_Entry entry = {depth, (original_alpha == alpha) ? AT_MOST : EXACT,
                    best_move, alpha, zobrist_hash};
  TT_TABLE[zobrist_hash % TT_SIZE] = entry;
  return alpha;
}

int16_t stableSearch(Board *board, int16_t alpha, int16_t beta) {
  Move moveList[MAX_CAPTURE_MOVES];
  int16_t evaluation = evaluate(board);
  if (evaluation >= beta) {
    return evaluation;
  }
  alpha = std::max<int16_t>(alpha, evaluation);
  uint16_t total_moves = board->generateCaptureMoves(moveList);
  if (total_moves == 0) {
    return evaluation;
  }
  std::sort(moveList, moveList + total_moves, [board](Move a, Move b) {
    return mvv_lva_heuristic(board, a) > mvv_lva_heuristic(board, b);
  });
  for (uint8_t move = 0; move < total_moves; ++move) {
    UndoInfo undo_info = board->playMove(moveList[move]);
    evaluation = -stableSearch(board, -beta, -alpha);
    board->undoMove(undo_info);
    if (evaluation >= beta) {
      return beta;
    }
    alpha = std::max<int16_t>(alpha, evaluation); // same perf. comp. here
  }
  return alpha;
}
