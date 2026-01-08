#include "board_manager.hpp"
#include "move_encoding.hpp"
#include <iostream>

using namespace std;

uint64_t convert_square_to_bitboard(uint8_t square1, uint8_t square2,
        uint8_t square3){
    return (1ULL << square1) | (1ULL << square2) | (1ULL << square3);
}

int main(){
 //Test 0 initial positions
 
    {
      Board board = Board();
      Move move = ENCODE_MOVE(18, 1, 0, 0);
      Board board_copy = Board();
      UndoInfo undo_info = board_copy.playMove(move);
      board_copy.undoMove(undo_info);
      if(board == board_copy){
        cout << "Test 0 PASSED" << endl;  
      } else {
        cout << "Test 0 FAILED" << endl;
        return 1;
      }
    }
    
 // Test 1: Pawn promotion position
    // White pawn on g7, can promote to 4 pieces = 4 moves
    {
        uint64_t pawns = convert_square_to_bitboard(49, 49, 49);      // g7
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = convert_square_to_bitboard(3, 3, 3);       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = convert_square_to_bitboard(59, 59, 59); // e8
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1; // white to move
        Move previous_move = Move(); 
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        Move move = ENCODE_MOVE(57, 49, 0, QUEEN_PIECE);
        Board board_copy(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        UndoInfo undo_info = board_copy.playMove(move);
        board_copy.undoMove(undo_info);
        if(board == board_copy){
          cout << "Test 1 PASSED" << endl;  
        } else {
          cout << "Test 1 FAILED" << endl;
          return 1;
        }
    }
    
    // Test 2: Pawn can capture-promote on both sides
    // White pawn on g7, black pieces on f8 and h8
    {
        uint64_t pawns = convert_square_to_bitboard(49, 49, 49);     // g7
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = convert_square_to_bitboard(3, 3, 3);       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = convert_square_to_bitboard(58, 56, 56); // f8, h8
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = convert_square_to_bitboard(59, 59, 59);   // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        Move move = ENCODE_MOVE(58, 49, 0, QUEEN_PIECE);
        Board board_copy(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        UndoInfo undo_info = board_copy.playMove(move);
        board_copy.undoMove(undo_info);
        if(board == board_copy){
          cout << "Test 2 PASSED" << endl;  
        } else {
          cout << "Test 2 FAILED" << endl;
          return 1;
        }
    }
    
    // Test 3: En passant capture available
    // White pawn on e5, black pawn just moved d7-d5
    {
        uint64_t pawns = convert_square_to_bitboard(35, 35, 35);      // e5
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = 0x0000000000000000ULL;
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = convert_square_to_bitboard(3, 3, 3);     // e1
        
        uint64_t enemy_pawns = convert_square_to_bitboard(36, 36, 36); // d5
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = convert_square_to_bitboard(59, 59, 59);  // e8
        
        uint8_t castle_state[2] = {CASTLE_NO_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move(); // set this to d7-d5 in your Move encoding
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        Move move = ENCODE_MOVE(44, 35, 1, 0);
        Board board_copy(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        UndoInfo undo_info = board_copy.playMove(move);
        board_copy.undoMove(undo_info);
        if(board == board_copy){
          cout << "Test 3 PASSED" << endl;  
        } else {
          cout << "Test 3 FAILED" << endl;
          return 1;
        }
        
    } 
    
    // Test 4: Castling kingside available
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = convert_square_to_bitboard(0, 0, 0);      // h1
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = convert_square_to_bitboard(3, 3, 3);       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = convert_square_to_bitboard(59, 59, 59);  // e8
       
        //white can castle kingside
        uint8_t castle_state[2] = {CASTLE_SHORT_NO_LONG, CASTLE_NO_SHORT_NO_LONG};
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        Move move = ENCODE_MOVE(1, 3, 0, 0);
        Board board_copy(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        UndoInfo undo_info = board_copy.playMove(move);
        board_copy.undoMove(undo_info);
        if(board == board_copy){
          cout << "Test 4 PASSED" << endl;
        } else {
          cout << "Test 4 FAILED" << endl;
          return 1;
        } 
    }
    
    // Test 5: Castling queenside available
    {
        uint64_t pawns = 0x0000000000000000ULL;
        uint64_t knights = 0x0000000000000000ULL;
        uint64_t bishops = 0x0000000000000000ULL;
        uint64_t rooks = convert_square_to_bitboard(7, 7, 7);      // a1
        uint64_t queen = 0x0000000000000000ULL;
        uint64_t king = convert_square_to_bitboard(3, 3, 3);       // e1
        
        uint64_t enemy_pawns = 0x0000000000000000ULL;
        uint64_t enemy_knights = 0x0000000000000000ULL;
        uint64_t enemy_bishops = 0x0000000000000000ULL;
        uint64_t enemy_rooks = 0x0000000000000000ULL;
        uint64_t enemy_queen = 0x0000000000000000ULL;
        uint64_t enemy_king = convert_square_to_bitboard(59, 59, 59);  // e8
        
        uint8_t castle_state[2] = {CASTLE_LONG_NO_SHORT, CASTLE_NO_SHORT_NO_LONG}; 
        // white can castle queenside
        uint8_t turn = 1;
        Move previous_move = Move();
        
        Board board(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        Move move = ENCODE_MOVE(5, 3, 0, 0);
        Board board_copy(pawns, knights, bishops, rooks, queen, king,
                   enemy_pawns, enemy_knights, enemy_bishops, enemy_rooks, enemy_queen, enemy_king,
                   castle_state, turn, previous_move);
        UndoInfo undo_info = board_copy.playMove(move);
        board_copy.undoMove(undo_info);
        if(board == board_copy){
          cout << "Test 5 PASSED" << endl;  
        } else {
          cout << "Test 5 FAILED" << endl;
          return 1;
        }    
    }  
}
