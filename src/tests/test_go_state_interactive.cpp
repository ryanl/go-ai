/* Test go state. When the user confirms that a result is correct,
   that result is saved to disk and used for automated testing later. */

#include <cstdio>
#include <iostream>
#include "../go_mechanics/go_state.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    const unsigned int board_size = BOARDSIZE;

    char board[board_size * board_size];

    cout << "Board size: " << BOARDSIZE << endl;

    TypeOfSuperko superko_types[3] = {
    SUPERKO_POSITIONAL,
    SUPERKO_SITUATIONAL,
    SUPERKO_NATURAL_SITUATIONAL
    };

    unsigned int superko_type_index = -1;

    while (superko_type_index > 3 || superko_type_index < 1) {
    cout << "What type of superko do you want to use?" << endl
         << "- 1. positional" << endl
         << "- 2. situational" << endl
         << "- 3. natural situational" << endl;

    cin >> superko_type_index;
    }

    TypeOfSuperko superko = superko_types[superko_type_index - 1];

    /*
    VanillaGoState vgs_start;
    VanillaGoGame vgg;
    vgg.superko =

    string turn = " ";
    while (turn != "B" && turn != "W") {
    cout << "Whose turn is it to play? (B or W): ";
    cin >> turn;
    }
    vgs_start.blackToPlay = (turn == "B");
    vgs_start.previousMoveWasPass = false;

    cout << endl << "Please enter the board configuration (B = BLACK, W = WHITE, [SPACE] = EMPTY)" << endl;

    unsigned int i = 0;

    cout << "  ";
    for (unsigned int x = 0; x < board_size; x++) {
    cout << (char)('a' + x) << " ";
    }

    for (unsigned int y = 0; y < board_size; y++) {
    printf("%2d ", y + 1);

    for (unsigned int x = 0; x < board_size; x++) {
        bool unfinished = true;
        do {
        int character = cin.get();
        cout << " ";

        switch (character) {
        case 'B': case 'W': case ' ':
            vgs_start.current_board_state.push_back((char)character);
            unfinished = false;
        default:
            cout << endl << "Unrecognised character" << endl;
            printf("%02d ", y+1);
            for (unsigned int j = 0 ; j < x; j++) {
            cout << board[(y * board_size) + j] << "  ";
            }
        }
        } while (unfinished);
    }
    cout << endl;
    }

    vgg.states.push_back(vgs_start);

    cout << "I'm loading that in to my program... " << endl;

    GoGameState ggs;
    ggs.loadGame(vgg); */

    GoState ggs = GoState::newGame(superko);

    while (true) {
    cout << "I have marked empty points which are illegal moves with # symbols" << endl;

    cout << "   ";

    for (unsigned int x = 0; x < board_size; x++) {
        cout << (char)('a' + x) << " ";
    }
    cout << endl;

    for (unsigned int y = 0; y < board_size; y++) {
        printf("%2d ", y+1);

        for (unsigned int x = 0; x < board_size; x++) {
        int colour = ggs.get(x, y);
        if (colour != EMPTY) {
            assert(!ggs.isValidMove(GoMove::move(x, y)));
            if (colour == BLACK) cout << "B";
            else if (colour == WHITE) cout << "W";
            else assert(false);
        } else {
            if (ggs.isValidMove(GoMove::move(x, y))) {
            cout << "_";
            } else {
            cout << "#";
            }
        }
        cout << " ";
        }
        cout << endl;
    }

    cout << "If this is not correct, please e-mail Ryan with the data you entered." << endl;

        string move = "";
    while (move == "") {
        switch (ggs.getNextToPlay()) {
        case BLACK: cout << "BLACK to play" << endl; break;
        case WHITE: cout << "WHITE to play" << endl; break;
        }

        cout << "Please enter a valid move (pass or e.g. c12): ";
        cin >> move;
        if (move == "pass") {
        ggs.makeMove(GoMove::pass());
        } else if (move.length() >= 2) {
        bool bad = false;

        unsigned int x = move.c_str()[0] - 'a';
        unsigned int y = 0;

        for (unsigned int i = 1; i < move.length(); i++) {
            if (move.c_str()[i] < '0' || move.c_str()[i] > '9') {
            bad = false;
            } else {
            y = (y * 10) + (move.c_str()[i] - '0');
            }
        }

        y -= 1; // numbering for users starts at 1 not zero

        bad = bad || x >= board_size || y >= board_size;

        if (!bad) {
            if (ggs.isValidMove(GoMove::move(x, y))) {
            ggs.makeMove(GoMove::move(x, y));
            } else {
            cout << "My program doesn't think that's a valid move." << endl;
            move = "";
            }
        } else {
            move = "";
        }
        } else {
        move = "";
        }
    }
    }
}
