# Chess Engine

A chess engine written in C++ featuring a bitboard based board representation, PEXT-based sliding piece attack generation, and a negamax search with alpha-beta pruning.

## Overview

FrostWeb has gone through two major changes. The original implementation used a 2D vector board representation with a basic minimax search and alpha beta pruning. The current implementation is a ground-up rewrite centered around bitboards and a significantly more powerful search.

## Architecture

### Board Representation

The original 2D vector implementation stored piece positions in an 8x8 array. Generating moves required iterating over the board and checking each square individually, which was slow and cache unfriendly.

The current implementation uses a bitboard representation where each piece type and color is stored as a 64 bit integer (uint64_t) with each bit corresponding to a square on the board. This allows move generation to exploit hardware level (__bultin_ctzll, __builtin_clzll) bit manipulation instructions rather than iterating over squares.

Sliding piece attacks (bishops, rooks, queens) are generated using PEXT (Parallel Bits Extract) instructions via the BMI2 instruction set extension for x86-64 architecture. Attack lookup tables are indexed by extracting the occupancy bits along each piece's attack ray, giving O(1) attack generation per piece with no branching. The attack bitboards are generated at compile time.

The switch from 2D vector to bitboard representation produced a 98.8% reduction in move generation, measured from the starting position.

### Move Generation

Legal moves are generated in two passes. A pseudo legal move list is generated first using bitboard operations, then filtered for legality by verifying the king is not left in check after each move. Check detection uses the same PEXT attack tables.

Move generation is verified correct through perft testing to depth 7 from the starting position, matching Stockfish's node counts exactly.

Special moves handled include castling (both kingside and queenside), en passant, and all four promotion pieces. En passant pin detection is handled correctly in the legality filter.

### Search

The search uses negamax with alpha-beta pruning. A quiescence search extends the main search at leaf nodes, searching all captures until a quiet position is reached.

### Move Ordering

Move ordering uses MVV-LVA (Most Valuable Victim, Least Valuable Attacker) to prioritize captures. Captures are scored by the difference between the victim's value and the attacker's value, ensuring winning captures like pawn takes queen are searched before losing captures. Quiet moves are left in generation order. This GREATLY increases the effets of alpha beta pruning.

MVV-LVA produced a 2.6x speedup at depth 5, measured from the starting position. At depth 7 the speedup was 5.4x, consistent with the expected compounding effect of better alpha-beta pruning at greater depth.

### Transposition Tables

I added a table that caches recently evaluated positions to improve the speed of the search by eliminating already searched position. However, if the position has been searched at an equal or higher depth then the cached value is simply returned by the search function, otherwise, the cached data is used in move ordering to further improve the effectiveness of Alpha Beta Pruning.

### Iterative Deepening

Iterative deepending searches the game tree at all depths before the target depth to populate the cache with valuable information. This is shown to be more efficient because the time it takes to search depths 1 ... n - 1 is negligible compared to searching at depth n.

### Evaluation

The evaluation function uses material values and piece-square tables. Each piece type has a 64-entry table encoding positional bonuses and penalties, encouraging central control, piece development, and king safety at a basic level.

## Performance

| Metric | Value |
|--------|-------|
| Perft depth 7 (starting position) | Correct |
| 2D vector to bitboard speedup | 98.8% reduction in move generation time |
| MVV-LVA speedup at depth 5 | 2.6x |
| MVV-LVA speedup at depth 7 | 5.4x |

## Planned Work

- SIMD vectorization of evaluation (AVX2)
- Parallel search with OpenMP
- Distributed search over RDMA

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

Requires a CPU with BMI2 support for PEXT instructions. Compile with `-mbmi2 -O2` or higher.
