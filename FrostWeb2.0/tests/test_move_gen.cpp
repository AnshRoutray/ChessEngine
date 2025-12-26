#include "board_manager.hpp"
#include "move_encoding.hpp"
#include <iostream>

using namespace std;

int main(){
    Board board;
    Move move_list[MAX_MOVES];
    int num = board.generateLegalMoves(move_list);
    cout << "Number of moves: " << num << endl;
    for(int i = 0; i < num; i++){
        cout << "From :" << (int)GET_FROM_SQUARE(move_list[i]) << ", To: " << (int)GET_TO_SQUARE(move_list[i]) << endl;
    }
    if(num != 20){
        return 1;
    }
    return 0;
}