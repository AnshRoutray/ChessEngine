#include "search.hpp"
#include "board_manager.hpp"
#include "evaluate.hpp"
#include "move_encoding.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>

TT_Entry TT_TABLE[TT_SIZE];
thread_local Move moveList[MAX_DEPTH][MAX_MOVES];
thread_local std::pair<Move, int16_t> newMoveList[MAX_DEPTH][MAX_MOVES];

Move retrieveBestMove(Board *board, uint8_t max_depth) {
  Move best_move = 0;
  for (int depth = 1; depth <= max_depth; depth++) {
    best_move = retrieveBestMoveAtDepth(board, depth);
  }
  return best_move;
}

Move retrieveBestMoveAtDepth(Board *board, uint8_t depth) {
  uint16_t total_moves = board->generateLegalMoves(moveList[depth]);
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
    UndoInfo undo_info = board->playMove(moveList[depth][move]);
    int16_t score = (depth == 0) ? -stableSearch(board, -INF, INF)
                                 : -searchBestMove(board, depth - 1, -INF, INF);
    board->undoMove(undo_info);
    if (score > max_score) {
      max_score = score;
      best_move = move;
    }
  }
  return moveList[depth][best_move];
}

int16_t searchBestMove(Board *board, uint8_t depth, int16_t alpha,
                       int16_t beta) {
  uint64_t zobrist_hash = board->zobrist_hash;
  TT_Entry tt_value = TT_TABLE[zobrist_hash & (TT_SIZE - 1)];
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
  uint16_t total_moves = board->generateLegalMoves(moveList[depth]);
  if (total_moves == 0) {
    if (board->is_square_attacked()) {
      return -(30000 - depth);
    } else {
      return 0;
    }
  }
  int16_t original_alpha = alpha;
  for (int i = 0; i < total_moves; i++) {
    newMoveList[depth][i] = {moveList[depth][i],
                             mvv_lva_heuristic(board, moveList[depth][i])};
  }
  std::sort(newMoveList[depth], newMoveList[depth] + total_moves,
            [board, tt_value, use_TT_best_move](std::pair<Move, int16_t> a,
                                                std::pair<Move, int16_t> b) {
              if (use_TT_best_move && (a.first == tt_value.best_move ||
                                       b.first == tt_value.best_move)) {
                return a.first == tt_value.best_move;
              }
              return a.second > b.second;
            });
  Move best_move = newMoveList[depth][0].first;
  int16_t current_score = -INF;
  for (uint16_t move = 0; move < total_moves; ++move) {
    UndoInfo undo_info = board->playMove(newMoveList[depth][move].first);
    int16_t score = (depth == 0)
                        ? -stableSearch(board, -beta, -alpha)
                        : -searchBestMove(board, depth - 1, -beta, -alpha);
    board->undoMove(undo_info);
    if (score >= beta) {
      TT_Entry entry = {depth, AT_LEAST, newMoveList[depth][move].first, beta,
                        zobrist_hash};
      TT_TABLE[zobrist_hash & (TT_SIZE - 1)] = entry;
      return beta;
    }
    if (score > current_score) {
      best_move = newMoveList[depth][move].first;
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
  Move captureMoveList[MAX_CAPTURE_MOVES];
  int16_t evaluation = evaluate(board);
  if (evaluation >= beta) {
    return evaluation;
  }
  alpha = std::max<int16_t>(alpha, evaluation);
  uint16_t total_moves = board->generateCaptureMoves(captureMoveList);
  if (total_moves == 0) {
    return evaluation;
  }
  std::sort(captureMoveList, captureMoveList + total_moves,
            [board](Move a, Move b) {
              return mvv_lva_heuristic(board, a) > mvv_lva_heuristic(board, b);
            });
  for (uint8_t move = 0; move < total_moves; ++move) {
    UndoInfo undo_info = board->playMove(captureMoveList[move]);
    evaluation = -stableSearch(board, -beta, -alpha);
    board->undoMove(undo_info);
    if (evaluation >= beta) {
      return beta;
    }
    alpha = std::max<int16_t>(alpha, evaluation); // same perf. comp. here
  }
  return alpha;
}
