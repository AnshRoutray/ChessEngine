// tests/test_move_gen.cpp
#include "board_manager.hpp"
#include "move_encoding.hpp"
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

int main() {
    bool allPassed = true;
    
    // Test 1: Pawn promotion position
    // White pawn on g7, can promote to 4 pieces = 4 moves
    {
        uint64_t pawns = 0x0000400000000000ULL;      // g7
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1; // white to move
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Pawn promotion (straight)", board, 9); // 4 promotions + king moves
    }
    
    // Test 2: Pawn can capture-promote on both sides
    // White pawn on g7, black pieces on f8 and h8
    {
        uint64_t pawns = 0x0000400000000000ULL;      // g7
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0500000000000000ULL; // f8, h8
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Pawn capture-promotions", board, 13); // 4+4+4 promotions + 1 king
    }
    
    // Test 3: En passant capture available
    // White pawn on e5, black pawn just moved d7-d5
    {
        uint64_t pawns = 0x0000000008000000ULL;      // e5
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000010000000ULL; // d5
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move(); // set this to d7-d5 in your Move encoding
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("En passant available", board, 3); // e5-e6, e5xd6 (en passant), king move
    }
    
    // Test 4: Knight in corner (only 2 moves)
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000001ULL;    // h1
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Knight in corner", board, 10); // 2 knight moves + 8 king moves
    }
    
    // Test 5: Knight in center (maximum mobility - 8 moves)
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000010000000ULL;    // d4
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Knight in center", board, 16); // 8 knight moves + 8 king moves
    }
    
    // Test 6: Castling kingside available
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000001ULL;      // h1
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
       
        //white can castle kingside
        uint8_t castle_state[2] = {CASTLE_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Kingside castling available", board, 20); // lots of rook+king moves + O-O
    }
    
    // Test 7: Castling queenside available
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000080ULL;      // a1
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_LONG_NO_SHORT, CASTLE_NO_SHORT_NO_LONG}; 
        // white can castle queenside
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Queenside castling available", board, 22); // rook+king moves + O-O-O
    }
    
    // Test 8: Bishop on open diagonal
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000010000000ULL;    // d4
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Bishop on open diagonal", board, 21); // 13 bishop moves + 8 king moves
    }
    
    // Test 9: Rook on open file
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000010000000ULL;      // d4
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Rook on open board", board, 22); // 14 rook moves + 8 king moves
    }
    
    // Test 10: Queen in center (maximum mobility)
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000010000000ULL;      // d4
        uint64_t king = 0x0000000000000008ULL;       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = 0x0800000000000000ULL;  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        
        allPassed &= testPosition("Queen in center", board, 35); // 27 queen moves + 8 king moves
    }   
    
    if (allPassed) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}
