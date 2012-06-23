#include "assert.h"
#include "genetic_algorithm.hpp"
#include "../interface_gtp/generic/gtp_parser_callbacks.hpp" // for intToString etc.

using namespace std;

class GA_ProblemDefinition_MaxF : public GA_ProblemDefinition {
private:
    RNG rng;

    vector<GA_GeneType*> gene_types;

public:
    GA_ProblemDefinition_MaxF() {
        GA_GeneTypeBoundedFloat* gtbf = new GA_GeneTypeBoundedFloat("1");

        gene_types.push_back(
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
        TicTacToeGame game;
        bool a_to_play = rng.getBool();

        while (!game.isDraw()) {
            unsigned int move_x, move_y;

            if (a_to_play) {
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

int main() {
    GA_ProblemDefinition_TicTacToe problem;

    GA_Solver solver(problem);

    unsigned int fight_survivors = 10, fight_hp = 5, final_fight_hp = 500;
    unsigned int mutants = 10, pop_size = 100;

    std::cout << "Creating initial random population\n";
    solver.generateInitialPopulation(pop_size);

    testOutOrganism(problem, solver.get(0));

    for (unsigned int i = 0; i < 500; i++) {
        std::cout << "Generation " << (i + 1) << "\n";

        solver.shufflePopulation();

        std::cout << "Predating... " << std::flush;
        solver.predateSome(10);

        //std::cout << "Fighting... " << std::flush;
        //solver.fightSome(fight_survivors, fight_hp);

        testOutOrganism(problem, solver.get(0));

        std::cout << "Breeding... " << std::flush;
        solver.breedSome(pop_size);

        std::cout << "Shuffling... " << std::flush;
        solver.shufflePopulation(); // this lets us lop off the end if we got too many draws without being unfair

        std::cout << "Mutating... " << std::flush;
        solver.mutateSome(mutants);

        std::cout << "\n";
    }
    solver.fightSome(1, final_fight_hp); // pick 1 to be the champion
    testOutOrganism(problem, solver.get(0));

    return 0;
}

