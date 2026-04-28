#pragma once
#include <array>
#include <cstdint>

constexpr uint64_t compute_diagonal(int sq) {
  uint64_t mask = 0;
  int rank = sq / 8;
  int file = sq % 8;
  for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--)
    mask |= 1ULL << (r * 8 + f);
  for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++)
    mask |= 1ULL << (r * 8 + f);
  for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++)
    mask |= 1ULL << (r * 8 + f);
  for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--)
    mask |= 1ULL << (r * 8 + f);
  return mask;
}

constexpr uint64_t compute_column(int file) {
  uint64_t mask = 0;
  for (int rank = 0; rank < 8; rank++)
    mask |= 1ULL << (rank * 8 + file);
  return mask;
}

constexpr uint64_t compute_rank(int rank) {
  uint64_t mask = 0;
  for (int file = 0; file < 8; file++)
    mask |= 1ULL << (rank * 8 + file);
  return mask;
}

constexpr uint64_t compute_knight_attacks(int sq) {
  uint64_t mask = 0;
  int rank = sq / 8;
  int file = sq % 8;
  if (rank + 2 < 8 && file + 1 < 8)
    mask |= 1ULL << ((rank + 2) * 8 + file + 1);
  if (rank + 2 < 8 && file - 1 >= 0)
    mask |= 1ULL << ((rank + 2) * 8 + file - 1);
  if (rank - 2 >= 0 && file + 1 < 8)
    mask |= 1ULL << ((rank - 2) * 8 + file + 1);
  if (rank - 2 >= 0 && file - 1 >= 0)
    mask |= 1ULL << ((rank - 2) * 8 + file - 1);
  if (rank + 1 < 8 && file + 2 < 8)
    mask |= 1ULL << ((rank + 1) * 8 + file + 2);
  if (rank + 1 < 8 && file - 2 >= 0)
    mask |= 1ULL << ((rank + 1) * 8 + file - 2);
  if (rank - 1 >= 0 && file + 2 < 8)
    mask |= 1ULL << ((rank - 1) * 8 + file + 2);
  if (rank - 1 >= 0 && file - 2 >= 0)
    mask |= 1ULL << ((rank - 1) * 8 + file - 2);
  return mask;
}

constexpr uint64_t compute_king_attacks(int sq) {
  uint64_t mask = 0;
  int rank = sq / 8;
  int file = sq % 8;
  for (int dr = -1; dr <= 1; dr++) {
    for (int df = -1; df <= 1; df++) {
      if (dr == 0 && df == 0)
        continue;
      int r = rank + dr;
      int f = file + df;
      if (r >= 0 && r < 8 && f >= 0 && f < 8)
        mask |= 1ULL << (r * 8 + f);
    }
  }
  return mask;
}

constexpr std::array<uint64_t, 64> generate_knight_attacks() {
  std::array<uint64_t, 64> result{};
  for (int i = 0; i < 64; i++)
    result[i] = compute_knight_attacks(i);
  return result;
}

constexpr std::array<uint64_t, 64> generate_king_attacks() {
  std::array<uint64_t, 64> result{};
  for (int i = 0; i < 64; i++)
    result[i] = compute_king_attacks(i);
  return result;
}

constexpr std::array<uint64_t, 8> generate_columns() {
  std::array<uint64_t, 8> result{};
  for (int i = 0; i < 8; i++)
    result[i] = compute_column(i);
  return result;
}

constexpr std::array<uint64_t, 8> generate_ranks() {
  std::array<uint64_t, 8> result{};
  for (int i = 0; i < 8; i++)
    result[i] = compute_rank(i);
  return result;
}

constexpr std::array<uint64_t, 64> generate_diagonals() {
  std::array<uint64_t, 64> result{};
  for (int i = 0; i < 64; i++)
    result[i] = compute_diagonal(i);
  return result;
}

constexpr auto KNIGHT_ATTACKS = generate_knight_attacks();
constexpr auto KING_ATTACKS = generate_king_attacks();
constexpr auto COLUMN = generate_columns();
constexpr auto RANK = generate_ranks();
constexpr auto DIAGONALS = generate_diagonals();

// Maximum number of squares attacked by queen or bishop is 13 2^13 = 8192
// (Could be 13 - 4 = 9)
extern uint64_t DIAGONAL_ATTACKS[64][8192];

// Maximum number of squares attacked by queen or rook is 14 2^14 = 16384 (Could
// be 14 - 4 = 10)
extern uint64_t STRAIGHT_ATTACKS[64][16384];

// Diagonal Attacks
void init_diagonal_attack_lookup_table();

// Straight attacks
void init_straight_attack_lookup_table();
