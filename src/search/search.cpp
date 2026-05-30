#include "search.hpp"
#include "board_manager.hpp"
#include "evaluate.hpp"
#include "move_encoding.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <omp.h>

TT_Entry TT_TABLE[TT_SIZE];
static std::atomic<bool> stop_worker_thread{false};
thread_local Move moveList[MAX_DEPTH][MAX_MOVES];
thread_local std::pair<Move, int16_t> newMoveList[MAX_DEPTH][MAX_MOVES];
thread_local Move killers[MAX_DEPTH][2];
thread_local int32_t history[2][64][64];

constexpr int16_t CAPTURE_BONUS = 10000;
constexpr int16_t KILLER_0_SCORE = 5000;
constexpr int16_t KILLER_1_SCORE = 4000;
constexpr int32_t HISTORY_MAX = KILLER_1_SCORE - 1;

Move retrieveBestMove(Board *board, uint8_t max_depth) {
  Move best_move = 0;
  stop_worker_thread.store(false, std::memory_order_relaxed);
#pragma omp parallel
  {
    Board thread_local_board = *board;
    int tId = omp_get_thread_num();
    if (tId == 0) {
      for (int depth = 1; depth <= max_depth; depth++) {
        best_move = retrieveBestMoveAtDepth(&thread_local_board, depth);
      }
      stop_worker_thread.store(true, std::memory_order_relaxed);
    } else {
      int starting_depth = (max_depth / 2) + tId % (max_depth / 2);
      for (int depth = starting_depth; depth <= max_depth; depth++) {
        retrieveBestMoveAtDepth(&thread_local_board, depth);
      }
      while (!stop_worker_thread.load(std::memory_order_relaxed)) {
        retrieveBestMoveAtDepth(&thread_local_board, max_depth);
      }
    }
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
  if (stop_worker_thread.load(std::memory_order_relaxed)) {
    return 0;
  }
  uint64_t zobrist_hash = board->zobrist_hash;
  uint64_t tt_data;
  Move tt_best_move = 0;
  bool use_TT_best_move = false;
  if (tt_probe(zobrist_hash, tt_data)) {
    tt_best_move = tt_move(tt_data);
    if (tt_depth(tt_data) >= depth) {
      CUTOFF_FLAG flag = tt_flag(tt_data);
      int16_t score = tt_score(tt_data);
      if (flag == EXACT) {
        return score;
      } else if (flag == AT_LEAST) {
        if (score >= beta) {
          return beta;
        }
      } else {
        if (score <= alpha) {
          return alpha;
        }
      }
    }
    use_TT_best_move = true;
  }

  constexpr uint8_t NMP_R = 3;
  bool in_check = board->is_square_attacked();
  bool has_non_pawn =
      (board->knights | board->bishops | board->rooks | board->queen) != 0;
  if (!in_check && depth >= NMP_R + 1 && has_non_pawn &&
      board->previous_move != 0 && evaluate(board) >= beta) {
    UndoInfo null_undo = board->playNullMove();
    int16_t null_score =
        -searchBestMove(board, depth - 1 - NMP_R, -beta, -beta + 1);
    board->undoNullMove(null_undo);
    if (null_score >= beta) {
      tt_store(zobrist_hash, depth, AT_LEAST, 0, beta);
      return beta;
    }
  }

  uint16_t total_moves = board->generateLegalMoves(moveList[depth]);
  if (total_moves == 0) {
    if (in_check) {
      return -(30000 - depth);
    } else {
      return 0;
    }
  }
  int16_t original_alpha = alpha;
  for (int i = 0; i < total_moves; i++) {
    Move m = moveList[depth][i];
    uint8_t captured = board->piece_locations[GET_TO_SQUARE(m)];
    int16_t score;
    if (captured != EMPTY_PIECE) {
      score = CAPTURE_BONUS + mvv_lva_heuristic(board, m);
    } else if (m == killers[depth][0]) {
      score = KILLER_0_SCORE;
    } else if (m == killers[depth][1]) {
      score = KILLER_1_SCORE;
    } else {
      int32_t h = history[board->turn][GET_FROM_SQUARE(m)][GET_TO_SQUARE(m)];
      score = (int16_t)std::min(h, HISTORY_MAX);
    }
    newMoveList[depth][i] = {m, score};
  }
  std::sort(newMoveList[depth], newMoveList[depth] + total_moves,
            [tt_best_move, use_TT_best_move](std::pair<Move, int16_t> a,
                                             std::pair<Move, int16_t> b) {
              if (use_TT_best_move &&
                  (a.first == tt_best_move || b.first == tt_best_move)) {
                return a.first == tt_best_move;
              }
              return a.second > b.second;
            });
  Move best_move = newMoveList[depth][0].first;
  int16_t current_score = -INF;
  constexpr uint16_t LMR_MOVE_THRESHOLD = 4;
  constexpr uint8_t LMR_MIN_DEPTH = 3;
  for (uint16_t move = 0; move < total_moves; ++move) {
    Move current_move = newMoveList[depth][move].first;
    UndoInfo undo_info = board->playMove(current_move);
    bool reduce = depth >= LMR_MIN_DEPTH && move >= LMR_MOVE_THRESHOLD &&
                  !in_check && undo_info.captured_piece == EMPTY_PIECE &&
                  GET_PROMOTION_PIECE(current_move) == 0;
    int16_t score;
    if (depth == 0) {
      score = -stableSearch(board, -beta, -alpha);
    } else if (reduce) {
      score = -searchBestMove(board, depth - 2, -alpha - 1, -alpha);
      if (score > alpha) {
        score = -searchBestMove(board, depth - 1, -beta, -alpha);
      }
    } else {
      score = -searchBestMove(board, depth - 1, -beta, -alpha);
    }
    board->undoMove(undo_info);
    if (score >= beta) {
      Move cutting_move = newMoveList[depth][move].first;
      bool is_capture =
          board->piece_locations[GET_TO_SQUARE(cutting_move)] != EMPTY_PIECE;
      bool is_ep = GET_EN_PASSANT_FLAG(cutting_move);
      if (!is_capture && !is_ep) {
        if (cutting_move != killers[depth][0]) {
          killers[depth][1] = killers[depth][0];
          killers[depth][0] = cutting_move;
        }
        history[board->turn][GET_FROM_SQUARE(cutting_move)]
               [GET_TO_SQUARE(cutting_move)] += depth * depth;
      }
      tt_store(zobrist_hash, depth, AT_LEAST, cutting_move, beta);
      return beta;
    }
    if (score > current_score) {
      best_move = newMoveList[depth][move].first;
      current_score = score;
    }
    alpha = std::max<int16_t>(alpha,
                              score); // Compare performance with if statement
  }
  tt_store(zobrist_hash, depth, (original_alpha == alpha) ? AT_MOST : EXACT,
           best_move, alpha);
  return alpha;
}

int16_t stableSearch(Board *board, int16_t alpha, int16_t beta) {
  Move captureMoveList[MAX_CAPTURE_MOVES];
  std::pair<Move, int16_t> scoredCaptureList[MAX_CAPTURE_MOVES];
  int16_t evaluation = evaluate(board);
  if (evaluation >= beta) {
    return evaluation;
  }
  alpha = std::max<int16_t>(alpha, evaluation);
  uint16_t total_moves = board->generateCaptureMoves(captureMoveList);
  if (total_moves == 0) {
    return evaluation;
  }
  for (uint16_t i = 0; i < total_moves; i++) {
    scoredCaptureList[i] = {captureMoveList[i],
                            mvv_lva_heuristic(board, captureMoveList[i])};
  }
  std::sort(
      scoredCaptureList, scoredCaptureList + total_moves,
      [](const std::pair<Move, int16_t> &a, const std::pair<Move, int16_t> &b) {
        return a.second > b.second;
      });
  for (uint8_t move = 0; move < total_moves; ++move) {
    UndoInfo undo_info = board->playMove(scoredCaptureList[move].first);
    evaluation = -stableSearch(board, -beta, -alpha);
    board->undoMove(undo_info);
    if (evaluation >= beta) {
      return beta;
    }
    alpha = std::max<int16_t>(alpha, evaluation);
  }
  return alpha;
}
