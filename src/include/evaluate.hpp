#pragma once

#include "board_manager.hpp"
#include <cstdint>

int16_t evaluate(Board *board);
int16_t mvv_lva_heuristic(Board *board, Move move);
void init_eval_tables();
