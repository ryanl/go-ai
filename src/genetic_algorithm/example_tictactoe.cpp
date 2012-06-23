#include "assert.h"
#include "genetic_algorithm.hpp"
#include "../interface_gtp/generic/gtp_parser_callbacks.hpp" // for intToString etc.

using namespace std;

// this isn't optimal (could do repeated squaring) but it's easy to read and doesn't need to be fast
unsigned int pow(unsigned int x, unsigned int y) {
    unsigned int ret = 1;

    while (y) {
        ret *= x;
        y--;
    }
    return ret;
}

unsigned int getBaseNDigit(unsigned int x, unsigned int n, unsigned int i) {
    return (x / pow(n, i)) % n;
}

unsigned int setBaseNDigit(unsigned int x, unsigned int n, unsigned int i, unsigned int d) {
    assert(d < n);

    unsigned int p = pow(n, i);
    unsigned int x_low = x % p;
    x /= p;
    x += d - (x % n);
    return (x * p) + x_low;
}

struct TicTacToeGame {
    char board[3][3];
    unsigned int empties;
    bool turn_is_x;

    TicTacToeGame() :
        empties(9),
        turn_is_x(true)
    {
        for (unsigned int x = 0; x < 3; x++) {
            for (unsigned int y = 0; y < 3; y++) {
                board[x][y] = ' ';
            }
        }
    }

    unsigned int countEmpties() const {
        return empties;
    }

    void flipColours() {
        for (unsigned int x = 0; x < 3; x++) {
            for (unsigned int y = 0; y < 3; y++) {
                if      (board[x][y] == 'X') board[x][y] = 'O';
                else if (board[x][y] == 'O') board[x][y] = 'X';
                else {
                    assert(board[x][y] == ' ');
                }
            }
        }
        turn_is_x = !turn_is_x;
    }

    void fromInteger(int b) {
        unsigned int board_values[3] = { ' ', 'X', 'O' };
        empties = 0;

        for (unsigned int x = 2; x < 3; x--) {
            for (unsigned int y = 2; y < 3; y--) {
                int c = b % 3;
                b /= 3;

                board[x][y] = board_values[c];
                if (board_values[c] == ' ') empties++;
            }
        }

        // TODO: calculate turn
    }

    unsigned int toInteger() const {
        unsigned int ret = 0;

        for (unsigned int x = 0; x < 3; x++) {
            for (unsigned int y = 0; y < 3; y++) {
                ret *= 3;
                if      (board[x][y] == 'X') ret += 1;
                else if (board[x][y] == 'O') ret += 2;
            }
        }

        return ret;
    }

    string toString() const {
        string ret;
        for (unsigned int y = 0; y < 3; y++) {
            for (unsigned int x = 0; x < 3; x++) {
                ret.push_back(board[x][y]);
            }
            ret.push_back('\n');
        }
        return ret;
    }

    bool isDraw() {
        return empties == 0;
    }

    bool playAndCheckForWin(unsigned int x, unsigned int y) {
        assert(x < 3 && y < 3);
        assert(board[x][y] == ' ');

        char turn = turn_is_x ? 'X' : 'O';
        board[x][y] = turn;
        empties--;
        turn_is_x = !turn_is_x;

        const int win_lines[8][3] = {
            {0,1,2},
            {3,4,5},
            {6,7,8},
            {0,3,6},
            {1,4,7},
            {2,5,8},
            {0,4,8},
            {2,4,6}
        };
        for (unsigned int l = 0; l < 8; l++) {
            bool match = true;

            for (unsigned int i = 0; i < 3; i++) {
                unsigned int mx = win_lines[l][i] % 3, my = win_lines[l][i] / 3;
                match = match && (board[mx][my] == turn);
            }

            if (match) return true;
        }

        return false;
    }
};

class GA_ProblemDefinition_TicTacToe : public GA_ProblemDefinition {
private:
    RNG rng;

    vector<GA_GeneType*> gene_types;

public:
    GA_ProblemDefinition_TicTacToe() {
        unsigned int option_count = 0;

        unsigned int gene_count = pow(3, 9); // about 20,000
        std::cout << "Gene count: " << gene_count << "\n";
        for (unsigned int i = 0; i < gene_count; i++) {
            TicTacToeGame game;
            game.fromInteger(i);
            assert(game.toInteger() == i);
            GA_GeneTypeEnum* gte = new GA_GeneTypeEnum(intToString(i));

            unsigned int count_added = 0;
            for (unsigned int j = 0; j < 9; j++) {
                if (game.board[j / 3][j % 3] == ' ') {
                    gte->addPossibleValue(intToString(j));
                    count_added++;
                    option_count++;
                }
            }
            assert(count_added == game.countEmpties());

            if (count_added != 0) {
                gene_types.push_back(gte);
            } else {
                // we can't use an enum as we have no options - we'll let this gene type be the trivial gene (i.e. no choices)
                delete gte;
                GA_GeneTypeTrivial* gtt = new GA_GeneTypeTrivial(intToString(i));
                gene_types.push_back(gtt);
            }
        }

        std::cout << "Average options per state: " << (float(option_count) / gene_count) << "\n";
    }

    const vector<GA_GeneType*>& getGeneTypes() const {
        return gene_types;
    }

    // order is important - a goes first
    // returns -1 if a wins, 1 if b wins
    GA_GameWinner playGameBetween(const vector<GeneValue_t>& a, const vector<GeneValue_t>& b) {
        TicTacToeGame game;

        bool a_to_play = true;
        while (!game.isDraw()) {
            unsigned int move = a_to_play ? stringToInt(boost::get<string>(a[game.toInteger()])) : stringToInt(boost::get<string>(b[game.toInteger()]));
            unsigned int move_x = move / 3, move_y = move % 3;

            assert( ( (GA_GeneTypeEnum*) gene_types[game.toInteger()] ) ->possible_values.size() == game.countEmpties());

            assert(move_x < 3 && move_y < 3);
            //std::cout << move_x << "," << move_y << " ";

            assert(game.board[move_x][move_y] == ' ');
            bool win = game.playAndCheckForWin(move_x, move_y);
            assert(game.board[move_x][move_y] == 'X');

            // check for win
            if (win) {
                return a_to_play ? GA_WIN_A : GA_WIN_B;
            }

            game.flipColours();

            a_to_play = !a_to_play;
        }

        return GA_DRAW;
    }

    // our predator is a random player
    GA_GameWinner playGameAgainstPredator(const std::vector<GeneValue_t>& a) {
        return playGameAgainstPredator(a, true);
    }

    GA_GameWinner playGameAgainstPredator(const std::vector<GeneValue_t>& a, bool no_random) {
        TicTacToeGame game;
        bool a_to_play = rng.getBool();

        while (!game.isDraw()) {
            unsigned int move_x, move_y;

            if (a_to_play && (no_random || rng.getIntBetween(0, 9) != 0)) {
                unsigned int move = stringToInt(boost::get<string>(a[game.toInteger()]));
                move_x = move / 3;
                move_y = move % 3;
            } else {
                do {
                    move_x = rng.getIntBetween(0, 2);
                    move_y = rng.getIntBetween(0, 2);
                } while (game.board[move_x][move_y] != ' ');
            }
            assert(move_x < 3 && move_y < 3);

            assert(game.board[move_x][move_y] == ' ');
            bool win = game.playAndCheckForWin(move_x, move_y);
            assert(game.board[move_x][move_y] == 'X');

            // check for win
            if (win) {
                return a_to_play ? GA_WIN_A : GA_WIN_B;
            }

            game.flipColours();
            a_to_play = !a_to_play;
        }

        return GA_DRAW;
    }
};

void testOutOrganism(GA_ProblemDefinition_TicTacToe &problem, const std::vector<GeneValue_t> a) {
    int wins = 0, draws = 0, losses = 0;
    for (unsigned int i = 0; i < 10000; i++) {
        GA_GameWinner res = problem.playGameAgainstPredator(a, true);
        if (res == GA_WIN_A) wins++;
        else if (res == GA_WIN_B) losses++;
        else draws++;
    }

    std::cout << "Wins: " << wins << ", draws: " << draws << ", losses: " << losses << "\n";
}

int main() {
    RNG rng;
    GA_ProblemDefinition_TicTacToe problem;

    std::vector<GA_Solver> solvers(1, problem);

    unsigned int fight_survivors = 5, fight_hp = 5, final_fight_hp = 500;
    unsigned int mutants = 5, pop_size = 15;

    for (unsigned int p = 0; p < solvers.size(); p++) {
        std::cout << "Creating initial random population " << p << "\n";
        solvers[p].addRandomIndividualsUntilSizeIs(pop_size);
        solvers[p].predateSome(fight_survivors);
    }

    testOutOrganism(problem, solvers[0].get(0));

    for (unsigned int i = 0; i < 500; i++) {
        std::cout << i << ": ";

        for (unsigned int p = 0; p < solvers.size(); p++) {
            solvers[p].breedSome(pop_size);

            solvers[p].mutateSome(mutants);
            /*
            if (p > 0 && rng.getIntBetween(0, 100) > 99) {
                std::cout << "!";
                //std::cout << "Complete extinction of population " << p << "\n";
                solvers[p].generateInitialPopulation(pop_size);
            }
            else if (p > 0 && rng.getIntBetween(0, 100) > 97) {
                std::cout << "?";
                //std::cout << "Complete mutation of population " << p << "\n";
                solvers[p].mutateSome(pop_size);
            } else {
                std::cout << " ";
            }
            */
            solvers[p].shufflePopulation();

            //std::cout << "Fighting... " << std::flush;
            // solvers[p].fightSome(fight_survivors, fight_hp);

            //std::cout << "Predating... " << std::flush;
            solvers[p].predateSome(fight_survivors);

            //testOutOrganism(problem, solvers[p].get(0));
            //std::cout << "Sending missionary\n";
            //for (unsigned int j = 0; j < solvers.size(); j++) {
            //    solvers[j].add(solvers[p].get(0));
            //}
            /*
            if (p == ((i / 5) % solvers.size())) {
                std::cout << ">";
                solvers[(p + 1) % solvers.size()].add(solvers[p].get(0));
            } else {
                std::cout << " ";
            }
            */
            //std::cout << "Breeding... " << std::flush;

            //std::cout << "Mutating... " << std::flush;
        }
        std::cout << "\n";
        testOutOrganism(problem, solvers[0].get(0));
    }
    solvers[0].fightSome(1, final_fight_hp); // pick 1 to be the champion
    testOutOrganism(problem, solvers[0].get(0));

    return 0;
}

