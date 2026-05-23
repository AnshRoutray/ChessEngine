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

### Pin-Aware Legality Filter

The original legality filter called is_square_attacked for every pseudo-legal move to verify the king was not left in check. Profiling showed this single function consumed 20% of total search time.

Most of those calls were wasted: a move can only expose the king if (a) the moving piece is the king, (b) the move is en passant (which removes a pawn off the king's rank), or (c) the moving piece is absolutely pinned to the king. Every other move is unconditionally legal.

compute_pinned_pieces() runs once per node and uses an x-ray technique on the king's sliding attacks to identify the bitboard of pinned friendly pieces, along with a one-shot is_square_attacked() check to determine whether we are in check. The legality loop then short circuits to a fast path for any move that is not pinned, not a king move, not en passant, and not in check.

This dropped is_square_attacked from 20% to under 4% of search time, with compute_pinned_pieces adding only ~3%. Net result: ~30% wall-clock speedup at depth 7.

### Null Move Pruning

Null Move Pruning (NMP) is a forward-pruning technique. Before the normal search at a node, we let the opponent move twice in a row (a "null move" on our side) and search the resulting position at reduced depth, depth - 1 - R, where R is the reduction. If even after giving up a tempo our score is still >= beta, we assume the full-depth search would also fail high and we cut off immediately.

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

The evaluation function uses material values and piece-square tables. Each piece type has a 64-entry table encoding positional bonuses and penalties, encouraging central control, piece development, and king safety at a basic level.

The evaluation is incremental: `play/undoMove` maintain a `pst_score_white` field on the Board by adding and subtracting per-piece PST contributions as pieces move, are captured, or promote. `evaluate()` is now O(1) and reduces to a single sign flip based on side-to-move. Previously, the evaluation iterated over every set bit in every piece bitboard and indexed into the PST tables for each, which was a meaningful cost given how often the leaves of the search tree call it.

This refactor also fixed a latent asymmetry bug: the previous score-from-side-to-move accumulator could return different values for the same position depending on who was to move. The new accumulator stores a single absolute score from white's perspective.

### Miscellaneous Optimizations

A few smaller changes contributed to the overall improvement:

- **One-shot table initialization.** Zobrist hash values, the diagonal and straight PEXT attack tables, and the evaluation tables were originally initialized inside the `Board` constructor. Profiling on short runs showed these initializations dominating wall-clock time. They were moved into a single `init_engine_tables()` called once in `main` instead of per `Board` instance.
- **Quiescence MVV-LVA pre-scoring.** The quiescence sort comparator was calling `mvv_lva_heuristic` twice per comparison (once for each operand). The capture moves are now pre scored into a paired array, eliminating the double evaluation.
- **Promotion dispatch table.** The promotion handling in `playMove` was a five-way `if/else` cascade across the four promotion piece types. It is now a small lookup table indexed by the promotion flag.

## Performance

Wall-clock time for a full search from the starting position, measured on the same machine in sequence:

| Stage | Depth 7 | Best move |
|-------|---------|-----------|
| Baseline (TT + MVV-LVA + iterative deepening) | 6.32 s | e2e4 |
| + pin-aware legality filter | 4.44 s | e2e4 |
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
- SIMD Vectorization of certain parts. (Most likely in an improved NN evaluation method)
- Parallel search with OpenMP
- Distributed Search over multiple computers using RDMA.

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

Requires a CPU with BMI2 support for PEXT instructions. Compile with `-mbmi2 -O2` or higher.
