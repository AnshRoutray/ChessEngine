# Chess Engine

A chess engine written in C++ featuring a bitboard based board representation, PEXT-based sliding piece attack generation, and a negamax search with alpha-beta pruning.

## Overview

This Chess Engine has gone through two major changes. The original implementation that I made in High School :) stored piece positions in an 8x8 integer vector. I used a 2D vector board representation with a basic minimax search and alpha beta pruning. The current implementation is a ground-up rewrite centered around bitboards and a significantly more powerful search. Honestly I'm surprised the old implementation even got to depth 5.

## Architecture

### Board Representation

The original 2D vector implementation required generating moves required iterating over the board and checking each square individually, which was EXTREMELY slow, imagine 64 integers being passed by copy (yes not even by reference) multiple times throughout a search tree.

The current implementation uses a bitboard representation where each piece type and color is stored as a 64 bit integer (uint64_t) with each bit corresponding to a square on the board. This allows move generation to exploit hardware level (__bultin_ctzll, __builtin_clzll) bit manipulation instructions rather than iterating over squares. So instead of a for loop it's literally ONE machine instruction.

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

### Improved Legality Filter

The original legality filter called is_square_attacked for every pseudo-legal move to verify the king was not left in check. Profiling suing perf showed this single function consumed 20% of total search time.

Most of those calls were wasted: a move can only expose the king if (a) the moving piece is the king, (b) the move is en passant (which removes a pawn off the king's rank), or (c) the moving piece is absolutely pinned to the king. Every other move is unconditionally legal.

This dropped is_square_attacked from 20% to under 4% of search time, with compute_pinned_pieces adding only 3%. Net result: 30% speedup at depth 7.

### Null Move Pruning

Null Move Pruning (NMP) is a forward pruning technique. Before the normal search at a node, we let the opponent move twice in a row and search the resulting position at reduced depth, depth - 1 - R, where R is the reduction. If even after giving up a tempo our score is still >= beta, we assume the full-depth search would also fail high and we cut off immediately.

This provided an approximately 25% speedup.

### Killer Moves

The transposition table provides position-specific move ordering, and MVV-LVA orders captures. Until killers, however, quiet (non-capture) moves were searched in arbitrary generation order. Since most beta cutoffs come from quiet moves at deeper depths, this was a significant gap.

A killer move is a quiet move that recently caused a beta cutoff at the current depth. The intuition is that positions visited at the same ply in the search tree tend to be similar: a defensive king move that escaped a tactic in one branch often works in a sibling branch as well.

With Killer moves, the time at depth 7 dropped from 3.25s (with NMP) to 1.05s which is a 3.1x improvement.

### History Heuristic

History is the second half of the quiet-move ordering problem. Killers track "what worked at this depth," but only the two most recent. History tracks "what has worked anywhere in the search" for every (color, from square, to square) combination, accumulated across the entire tree.

In this benchmark from the starting position, the marginal speedup from history was small (1%) because killers and the TT already cover most of the quiet move ordering needs of a symmetric opening. History's larger contribution shows up under LMR, where it provides per move quality signal for reduction decisions.

### Late Move Reductions

With move ordering now strong, the first few moves at each node are very likely to be the best ones, and exhaustively searching the remaining moves at full depth is mostly wasted effort.

LMR reduces the search depth for "late" moves, those beyond a move-index threshold, at sufficient node depth, that are quiet, not in check, and not a promotion. If the reduced search returns a score above alpha (suggesting the move might actually be good), we re-search it at full depth to get the true score.

This caused the time at depth 8 to be 1.18s returning the move e2e4.

### Evaluation

The evaluation function uses material values and piece-square tables. Each piece type has a 64-entry table encoding positional bonuses and penalties, encouraging central control, piece development, and king safety at a basic level. Planned to add a mroe sophisticated evaluation function, maybe a NN.


## Performance

Wall-clock time for a full search from the starting position, measured on the same machine in sequence:

| Stage | Depth 7 | Best move |
|-------|---------|-----------|
| Baseline (TT + MVV-LVA + iterative deepening) | 6.32 s | e2e4 |
| + improved legality filter | 4.44 s | e2e4 |
| + null move pruning (R=3) | 3.25 s | e2e4 |
| + killer moves | 1.05 s | e2e4 |
| + history heuristic | 1.04 s | e2e4 |

With LMR enabled the engine searches one ply deeper in comparable wall time:

| Stage | Depth 8 | Best move |
|-------|---------|-----------|
| + late move reductions | 1.18 s | e2e4 |

| Metric | Value |
|--------|-------|
| Perft depth 7 (starting position) | Correct |
| 2D vector to bitboard speedup | 98.8% reduction in move generation time |
| MVV-LVA speedup at depth 5 | 2.6x |
| MVV-LVA speedup at depth 7 | 5.4x |
| Full optimization stack (depth 7) | ~6x speedup over baseline |

## Planned Work

- Neural Network evaluation over Hand Crafted.
- Distributed Search over multiple computers using RDMA.

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Requires a CPU with BMI2 (PEXT) support, OpenMP, and `-mcmodel=medium` (all set in `CMakeLists.txt`). System must have at least 8 GB of RAM and 6 logical CPUs; the engine validates this at startup and exits with an error otherwise.

## Run

```bash
./chess_engine [depth]
```

`depth` defaults to 10. The transposition table is fixed at 8 GB. If you want to change, value is located in /include/search.hpp the TT_SIZE value. Thread count defaults to all available logical CPUs; override with `OMP_NUM_THREADS`:

```bash
OMP_NUM_THREADS=6 ./chess_engine 8
```
