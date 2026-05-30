#pragma once

#include "board_manager.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>

constexpr int16_t INF = INT16_MAX;
constexpr uint8_t MAX_CAPTURE_MOVES = 218;
constexpr uint8_t MAX_DEPTH = 40;

constexpr int TT_SIZE = 1 << 25;

enum CUTOFF_FLAG : uint8_t { EXACT = 0, AT_LEAST = 1, AT_MOST = 2 };

// Lockless TT
struct TT_Entry {
  std::atomic<uint64_t> key{0};  // zobrist_hash XOR data
  std::atomic<uint64_t> data{0}; // packed: depth | flag | move | score
};

static_assert(std::atomic<uint64_t>::is_always_lock_free,
              "Transposition Table requires lock-free 64-bit atomics");

extern TT_Entry TT_TABLE[TT_SIZE];

inline uint64_t tt_pack(uint8_t depth, CUTOFF_FLAG flag, Move move,
                        int16_t score) {
  return (uint64_t)depth | ((uint64_t)flag << 8) | ((uint64_t)move << 16) |
         ((uint64_t)(uint16_t)score << 32);
}

inline uint8_t tt_depth(uint64_t d) { return d & 0xFF; }
inline CUTOFF_FLAG tt_flag(uint64_t d) {
  return (CUTOFF_FLAG)((d >> 8) & 0xFF);
}
inline Move tt_move(uint64_t d) { return (Move)((d >> 16) & 0xFFFF); }
inline int16_t tt_score(uint64_t d) { return (int16_t)((d >> 32) & 0xFFFF); }

inline void tt_store(uint64_t hash, uint8_t depth, CUTOFF_FLAG flag, Move move,
                     int16_t score) {
  uint64_t data = tt_pack(depth, flag, move, score);
  std::size_t idx = hash & (TT_SIZE - 1);
  TT_TABLE[idx].key.store(hash ^ data, std::memory_order_relaxed);
  TT_TABLE[idx].data.store(data, std::memory_order_relaxed);
}

inline bool tt_probe(uint64_t hash, uint64_t &data_out) {
  std::size_t idx = hash & (TT_SIZE - 1);
  uint64_t key = TT_TABLE[idx].key.load(std::memory_order_relaxed);
  uint64_t data = TT_TABLE[idx].data.load(std::memory_order_relaxed);
  if ((key ^ data) != hash)
    return false;
  data_out = data;
  return true;
}

Move retrieveBestMove(Board *board, uint8_t depth);

Move retrieveBestMoveAtDepth(Board *board, uint8_t depth);

int16_t stableSearch(Board *board, int16_t alpha, int16_t beta);

// HANDLE case where this function returns 0 (which means board has no moves
// left)
int16_t searchBestMove(Board *board, uint8_t depth, int16_t alpha,
                       int16_t beta);
