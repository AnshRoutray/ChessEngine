#include "lookup_tables.hpp"
#include <array>
#include <cstdint>
#include <immintrin.h>
#include <iostream>

uint64_t DIAGONAL_ATTACKS[64][8192];
uint64_t STRAIGHT_ATTACKS[64][16384];

uint64_t get_diagonal_attack_bitboard(uint8_t square, uint16_t piece_layout) {
  uint64_t enemy_piece_layout = _pdep_u64(piece_layout, DIAGONALS[square]);
  uint64_t attack_bitboard = 0;
  // NorthEast
  uint8_t temp_index = square;
  while (temp_index < 56 && temp_index % 8 > 0) {
    temp_index += 7;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  // SouthWest
  temp_index = square;
  while (temp_index > 7 && temp_index % 8 < 7) {
    temp_index -= 7;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  // NorthWest
  temp_index = square;
  while (temp_index < 56 && temp_index % 8 < 7) {
    temp_index += 9;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  // SouthEast
  temp_index = square;
  while (temp_index > 7 && temp_index % 8 > 0) {
    temp_index -= 9;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  return attack_bitboard;
}

uint64_t get_straight_attack_bitboard(uint8_t square, uint16_t piece_layout) {
  uint64_t enemy_piece_layout =
      _pdep_u64(piece_layout, COLUMN[square % 8] ^ RANK[square / 8]);
  uint64_t attack_bitboard = 0;
  // North
  uint8_t temp_index = square;
  while (temp_index < 56) {
    temp_index += 8;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  // South
  temp_index = square;
  while (temp_index > 7) {
    temp_index -= 8;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  // West
  temp_index = square;
  while (temp_index % 8 < 7) {
    ++temp_index;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  // East
  temp_index = square;
  while (temp_index % 8 > 0) {
    --temp_index;
    attack_bitboard |= (1ULL << temp_index);
    if (enemy_piece_layout & (1ULL << temp_index))
      break;
  }
  return attack_bitboard;
}

void init_diagonal_attack_lookup_table() {

  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 8192; j++) {
      DIAGONAL_ATTACKS[i][j] = get_diagonal_attack_bitboard(i, j);
    }
  }
}

void init_straight_attack_lookup_table() {
  for (int i = 0; i < 64; i++) {
    for (int j = 0; j < 16384; j++) {
      STRAIGHT_ATTACKS[i][j] = get_straight_attack_bitboard(i, j);
    }
  }
}
