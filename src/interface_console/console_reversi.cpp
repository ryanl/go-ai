#include "../reversi_mechanics/reversi_state.hpp"
#include "../reversi_mechanics/reversi_move.hpp"
#include <iostream>

using namespace std;

int main_old(int argc, char* argv[]) {
    ReversiState s;

    cout << "REVERSI\n\n";

    while (!cin.eof()) {
        cout << "----------------------------\n"
             << s.humanReadableState();

        int x = -1, y = -1;

        vector<ReversiMove> options = s.getValidMoves();

        assert(options.size() > 0);

        cout << "Valid moves: ";


        for (size_t i = 0; i < options.size(); i++) {
            if (i > 0) {
                cout << ", ";
            }

            if (options[i].isPass()) {
                cout << "PASS";
            } else {
                int x = options[i].getX(), y = options[i].getY();
                cout << char('A' + y) << char('1' + x);
            }
        }


        while (true) {
            cout << "\n\nYour move? ";

            string move;
            cin >> move;

               if (cin.eof()) break;

            if (options[0].isPass() && move == "PASS") {
                s.makeForcedPass();
                break;
            } else if (move.length() != 2) {
                cout << "Bad syntax: moves should look like B4, you gave'" << move << "'\n";
            } else {
                x = move.c_str()[1] - '1';
                y = move.c_str()[0] - 'A';

                //cout << "(x,y)=(" << x << "," << y << ")\n";

                if (0 <= x && x < REVERSI_BOARD_SIZE &&
                    0 <= y && y < REVERSI_BOARD_SIZE) {

                    if (!s.isValidMove(x, y)) {
                        cout << "Invalid move\n";
                    } else {
                        s.makeMove(x, y);
                        break;
                    }
                } else {
                    cout << "(x,y)=(" << x << "," << y << ") out of range\n";
                }
            }
        }


    }

    cout << "\n";

    return 0;
}
