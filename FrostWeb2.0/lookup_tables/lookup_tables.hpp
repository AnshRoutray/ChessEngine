#pragma once
#include <cstdint>

// Rank and File masks

// Files (columns A-H from left to right)
constexpr uint64_t FILE[8];

// Ranks (rows 1-8 from bottom to top)
constexpr uint64_t RANK[8];

//Main Diagonals attack squares per each square (center square is set to 0)
constexpr uint64_t DIAGONALS[64];

// Knight attack lookup table
constexpr uint64_t KNIGHT_ATTACKS[64];;

//Diagonal Attacks lookup table for bishops and queens depending on piece layout
constexpr uint64_t DIAGONAL_ATTACKS[64][8192];

//Straight Attacks lookup table for rooks and queens depending on piece layout
constexpr uint64_t STRAIGHT_ATTACKS[64][16384];

//King Attacks from each square
constexpr uint64_t KING_ATTACKS[64];

//Diagonal Attacks
void init_diagonal_attack_lookup_table();

//Straight attacks
void init_straight_attack_lookup_table();