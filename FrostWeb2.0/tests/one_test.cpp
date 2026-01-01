// tests/test_move_gen.cpp
#include "board_manager.hpp"
#include "move_encoding.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>

bool testPosition(const char* description, Board& board, int expected) {
    Move move_list[MAX_MOVES];
    uint16_t count = board.generateLegalMoves(move_list);
    if (count != expected) {
        std::cerr << "FAILED: " << description << std::endl;
        std::cerr << "  Expected " << expected << " moves, got " << count << std::endl;
        return false;
    }
    
    std::cout << "PASSED: " << description << " (" << count << " moves)" << std::endl;
    return true;
}

uint64_t convert_square_to_bitboard(uint8_t square1, uint8_t square2,
        uint8_t square3){
    return (1ULL << square1) | (1ULL << square2) | (1ULL << square3);
}

int main() {
    bool allPassed = true;
    // Test 4: Knight in corner (only 2 moves)
    {
        uint64_t pawns = convert_square_to_bitboard(35, 35, 35);
        uint64_t knights = 0;    // h1
        uint64_t bishops = 0;
        uint64_t rooks = 0; 
        uint64_t queen = 0;
        uint64_t king = convert_square_to_bitboard(0, 0, 0);       // e1
        
        uint64_t enemy_pawns = convert_square_to_bitboard(36, 36, 36);
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = convert_square_to_bitboard(63, 63, 63);
        uint64_t enemy_rooks = 0;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = convert_square_to_bitboard(59, 59, 59);  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = ENCODE_MOVE(36, 52, 0, 0);
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("En Passant Pin", board, 4); // 2 knight moves + 8 king moves
    }

    
    if (allPassed) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}
