#undef NDEBUG

#include <set>
#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <vector>

#include "../go_mechanics/go_state.hpp"
#include "../interface_gtp/generic/external_gtp_process.hpp" // to connect to gnugo
#include "../interface_gtp/go_gtp_utils.hpp" // moveToString etc.

using namespace std;


void okay() {
    cout << " okay\n";
}

void checkBlank(GoState& s) {
    for (unsigned int x = 0; x < BOARDSIZE; x++) {
        for (unsigned int y = 0; y < BOARDSIZE; y++) {
            if (s.get(x, y) != EMPTY) {
                cout << "s.get(" << x << "," << y << ") = " << s.get(x, y) << "\n";
                assert(false);
            }
        }
    }
}

vector<std::string> splitBy(std::string s, char delim) {
    s.push_back(delim);

    vector<std::string> ret;
    unsigned int start = 0;

    for (unsigned int i = 0; i < s.length(); i++) {
        if (s[i] == delim) {
            std::string subs = s.substr(start, i - start);
            //std::cout << subs << ", " << std::flush;
            ret.push_back(subs);
            start = i + 1;
        }
    }

    return ret;
}

int main(int argc, char *argv[]) {
    GoState s = GoState::newGame(SUPERKO_POSITIONAL);

    cout << "------------------------------------------------------\n";
    cout << "Ryanbot regression test for go state related code\n";
    cout << "------------------------------------------------------\n";

    cout << "1. Checking BLACK, WHITE and EMPTY are three distinct values... " << flush;
    assert(BLACK != WHITE);
    assert(BLACK != EMPTY);
    assert(WHITE != EMPTY);
    okay();

    cout << "2. Checking BLACK is the first to play.. ." << flush;
    assert(s.getNextToPlay() == BLACK);
    checkBlank(s);
    okay();

    cout << "3. Checking every position on the board is a valid move, and so is pass... " << flush;
    assert(s.validMoves().size() == 1 + (BOARDSIZE * BOARDSIZE));
    assert(s.isValidMove(GoMove::pass()));

    for (unsigned int x = 0; x < BOARDSIZE; x++) {
        for (unsigned int y = 0; y < BOARDSIZE; y++) {
            assert(s.isValidMove(GoMove::move(x, y)));
        }
    }

    okay();

    unsigned int score_disagreements = 0;

    unsigned int games = 1000;
    cout << "4. Comparing results with gnugo's implementation of the rules over " << games << " games...\n";

    for (unsigned int i = 1; i <= games; i++) {
        cout << "Game #" << i << "/" << games << "\n";

        s = GoState::newGame(SUPERKO_POSITIONAL);
        std::string gnugo_and_args = "gnugo --mode gtp --chinese-rules --boardsize " + intToString(BOARDSIZE) + " --level 0 --positional-superko --komi 6.5 --play-out-aftermath --score aftermath --capture-all-dead";
        ExternalGTPProcess gnugo(gnugo_and_args);

        bool turn_black = true;

        s.setKomi(6.5f);
        assert(s.getKomi() == 6.5f);

        bool previous_move_pass = false;

        assert(s.getPreviousMove() == GoMove::none());

        for (unsigned int m = 0;; m++) {
            assert((s.getNextToPlay() == BLACK) == turn_black);
            assert(s.getPreviousMoveWasPass() == previous_move_pass);

            bool legal_move[BOARDSIZE][BOARDSIZE];
            for (unsigned int x = 0; x < BOARDSIZE; x++) {
                for (unsigned int y = 0; y < BOARDSIZE; y++) {
                    legal_move[x][y] = false;
                }
            }

            {
                std::string query = "all_legal ";
                if (turn_black) query += "b"; else query += "w";

                GTPResponse r = gnugo.sendGTPQuery(query);
                assert(r.isSuccess());

                //std::cout << r.getMsg() << "\n";

                vector<string> valid_moves = splitBy(r.getMsg(), ' ');
                for (unsigned int i = 0; i < valid_moves.size(); i++) {
                    if (valid_moves[i] != "") { // ignore padding
                        GoMove move = stringToMove(valid_moves[i]);
                        assert(move.isNormal());
                        legal_move[move.getX()][move.getY()] = true;
                    }
                }
            }

            // pass is always a legal move
            assert(s.isValidMove(GoMove::pass()));

            for (unsigned int x = 0; x < BOARDSIZE; x++) {
                for (unsigned int y = 0; y < BOARDSIZE; y++) {
                    bool ryanbot_thinks_legal = s.isValidMove(GoMove::move(x,y));

                    if (ryanbot_thinks_legal != legal_move[x][y]) {
                        std::cout << "Gnugo and Ryanbot disagree on move legality\n";

                        std::cout << "Gnugo showboard output:\n";
                        GTPResponse r_showboard = gnugo.sendGTPQuery("showboard");
                        assert(r_showboard.isSuccess());
                        std::cout << r_showboard.getMsg() << "\n";

                        std::cout << "Ryanbot boardToString:\n" << boardToString(s) << "\n";

                        std::cout << "Ryanbot: " << moveToString(GoMove::move(x,y)) << " legal: " << ryanbot_thinks_legal << "\n";
                        std::cout << "Gnugo:   " << moveToString(GoMove::move(x,y)) << " legal: " << legal_move[x][y] << "\n";

                        assert(false);
                    }
                }
            }

            std::string move_command = std::string("genmove ") + (turn_black ? "b" : "w");

            GTPResponse r = gnugo.sendGTPQuery(move_command);
              assert(r.isSuccess());

              std::cout << r.getMsg() << " " << std::flush;
              GoMove move = stringToMove(r.getMsg());

              if (move.isResign()) {
                  break;
              } else {
                  assert(s.isValidMove(move));
                  s.makeMove(move);

                turn_black = !turn_black;

                if (move.isPass()) {
                    if (previous_move_pass) {
                        break;
                    }
                    previous_move_pass = true;
                } else {
                    previous_move_pass = false;
                }

                assert(s.getPreviousMoveWasPass() == previous_move_pass);
            }
        }

        ScoredGame sg = s.scoreGame();

        char expected[10];
        if (sg.black_score > sg.white_score) {
            sprintf(expected, "B+%.1f", sg.black_score - sg.white_score);
            assert(s.getWinnerOfGame() == BLACK);
        } else if (sg.white_score > sg.black_score) {
            sprintf(expected, "W+%.1f", sg.white_score - sg.black_score);
            assert(s.getWinnerOfGame() == WHITE);
        } else {
            sprintf(expected, "DRAW");
            assert(s.getWinnerOfGame() == EMPTY);
        }

        std::string query = "final_score";
        GTPResponse r = gnugo.sendGTPQuery(query);
        assert(r.isSuccess());
        if (r.getMsg() != std::string(expected)) {
            std::cout << "Gnugo and Ryanbot disagree on scoring\n";

            std::cout << "Gnugo showboard output:\n";
            GTPResponse r_showboard = gnugo.sendGTPQuery("showboard");
            assert(r_showboard.isSuccess());
            std::cout << r_showboard.getMsg() << "\n";

            std::cout << "Gnugo score: " << r.getMsg() << "\n";

            std::cout << "Ryanbot boardToString:\n" << boardToString(s) << "\n";

            std::cout << "Ryanbot score: " << expected << "\n";

            score_disagreements ++;
        }

        std::cout << "\n";
    }

    if (score_disagreements) {
        cout << "\n" << "POSSIBLE PASS: Gnugo disagreed with the final score in " << score_disagreements << " of " << games << " games\n";
    } else {
        cout << "\n" << "PASSED" << "\n";
    }
}
