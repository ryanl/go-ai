#undef NDEBUG

#include <iostream>
#include <string>
#include <vector>
#include <queue>

#include "go_mechanics/go_state.hpp"
#include "interface_gtp/generic/external_gtp_process.hpp"
#include "interface_gtp/go_gtp_utils.hpp"
#include "genetic_algorithm.hpp"

using namespace std;


class GA_ProblemDefinition_GoParams : public GA_ProblemDefinition {
private:
    RNG rng;

    vector<GA_GeneType*> gene_types;

public:
    GA_GameWinner playGameBetween(const vector<GeneValue_t>& a, const vector<GeneValue_t>& b) {
        abort();
    }

    GA_GameWinner playGameAgainstPredator(const std::vector<GeneValue_t>& a) {
        abort();
    }

    GA_ProblemDefinition_GoParams() {

        gene_types.push_back(new GA_GeneTypeBoundedFloat("rave_weight_initial", 0.5, 2.0f, 0.01f, 0.1f));
        gene_types.push_back(new GA_GeneTypeBoundedFloat("rave_weight_final", 3.0, 10000.0f, 20.0f, 0.2f));
        gene_types.push_back(new GA_GeneTypeBoundedFloat("exploration", 0.0, 2.0f, 0.01f, 0.1f));
        gene_types.push_back(new GA_GeneTypeBoundedFloat("grandfather_heuristic_weighting", 0.0, 10.0f, 0.05f, 0.1f));

        {
            GA_GeneTypeEnum* no_rave = new GA_GeneTypeEnum("-no_rave");
            no_rave->addPossibleValue("-no_rave");
            no_rave->addPossibleValue("-no_weighted_rave");
            no_rave->addPossibleValue("-square_rave_weight 1");
            no_rave->addPossibleValue("");
            gene_types.push_back(no_rave);
        }
        {
            GA_GeneTypeEnum* no_patterns = new GA_GeneTypeEnum("-no_patterns");
            no_patterns->addPossibleValue("-no_patterns");
            no_patterns->addPossibleValue("");
            gene_types.push_back(no_patterns);
        }
        {
            GA_GeneTypeEnum* move_select = new GA_GeneTypeEnum("-move_select");
            move_select->addPossibleValue("-move_select times_played");
            move_select->addPossibleValue("-move_select value_estimate");
            //move_select->addPossibleValue("-move_select mean_wins");
            gene_types.push_back(move_select);
        }
        {
            GA_GeneTypeEnum* expansion_threshold = new GA_GeneTypeEnum("-expansion_threshold");
            expansion_threshold->addPossibleValue("-expansion_threshold 1");
            expansion_threshold->addPossibleValue("-expansion_threshold 2");
            expansion_threshold->addPossibleValue("-expansion_threshold 3");
            //expansion_threshold->addPossibleValue("-expansion_threshold 5");
            gene_types.push_back(expansion_threshold);
        }
        {
            GA_GeneTypeEnum* g = new GA_GeneTypeEnum("-rave_check_same");
            g->addPossibleValue("-rave_check_same 0");
            g->addPossibleValue("-rave_check_same 1");
            gene_types.push_back(g);
        }
        {
            GA_GeneTypeEnum* g = new GA_GeneTypeEnum("-rave_update_passes");
            g->addPossibleValue("-rave_update_passes 0");
            g->addPossibleValue("-rave_update_passes 1");
            gene_types.push_back(g);
        }
    }

    const vector<GA_GeneType*>& getGeneTypes() const {
        return gene_types;
    }
};

class GA_Solver_Go : public GA_Solver {

public:
    GA_Solver_Go(GA_ProblemDefinition &problem) :
        GA_Solver(problem)
    {}


    std::string buildCommandLineFor(const vector<GeneValue_t>& a) {
        std::string ret = "bin/go_gtp";
        GA_GeneTypeBoundedFloat dummyfloat("", 0, 0, 0, 0);
        GA_GeneTypeEnum dummyenum("");

        for (unsigned int i = 0; i < a.size(); i++) {
            if (typeid(*gene_types[i])== typeid(dummyenum)) {
                const std::string *v = boost::get<std::string>(&a[i]);
                assert(v);
                ret += " " + *v;
            } else  if (typeid(*gene_types[i]) == typeid(dummyfloat)) {
                const float *v = boost::get<float>(&a[i]);
                assert(v);
                ret += " -" + gene_types[i]->getParameterName() + " " + toString(*v);
            } else {
                std::cout << "Type id: '" << typeid(*gene_types[i]).name() << "'\n";

                assert(false);
                abort();
            }
        }
        return ret;
    }

    // order is important - a goes first
    redi::pstream* createMatch(const vector<GeneValue_t>& a, const vector<GeneValue_t>& b) {
        std::string cmd_a = buildCommandLineFor(a), cmd_b = buildCommandLineFor(b);

        std::string cmd = "bin/play_two_gtp_engines 1 40 1 --batch 2>/dev/null";

        std::cout << "!" << std::flush;

        redi::pstream* ret = new redi::pstream(cmd);

        *ret << cmd_a << "\n";
        *ret << cmd_b << "\n" << std::flush;
        ((redi::pstreambuf*) ret->rdbuf())->peof();

        return ret;
    }

    struct match {
        redi::pstream *p;
        unsigned int i, j;
    };

    void outputCommandLines() {
        for (unsigned int i = 0; i < population.size(); i++) {
            cout << i << ": " << buildCommandLineFor(population[i]) << "\n\n";
        }
    }

    void fightSome(unsigned int survivors, unsigned int start_hp) {
        std::vector<int> hp(population.size(), start_hp * 2);

        unsigned int max_trials = population.size() * start_hp * 3; // arbitrary

        for (unsigned int trial = 0; trial < max_trials && population.size() > survivors; trial++) {
            // choose two distinct individuals

            unsigned int a = rng.getIntBetween(0, population.size() - 1);
            unsigned int b = rng.getIntBetween(0, population.size() - 2);
            if (b >= a) b++;

            GA_GameWinner game_result = pd.playGameBetween(population[a], population[b]);

            int loser = -1, winner = -1;
            if (game_result == GA_WIN_A) {
                loser = b;
                winner = a;
            } else if (game_result == GA_WIN_B) {
                loser = a;
                winner = b;
            }

            if (loser != -1) {
                hp.at(winner)++; // compensate winner for the bad luck of being chosen to fight
                hp.at(loser) -= 2;
                if (hp.at(loser) <= 0) {
                    // kill of the loser if it has <= 0 HP
                    population.at(loser) = population[population.size() - 1];
                    hp.at(loser) = hp[population.size() - 1];

                    population.pop_back();
                    hp.pop_back();
                }
            }
        }

        // if we didn't get enough victims then we got a lot of draws so we will pick off some unlucky few
        while (population.size() > survivors) population.pop_back();
    }


    void tournamentSome(unsigned int tournament_survivors, unsigned int repeats) {
        vector<int> score(population.size(), 0);

        queue< match > matches_in_progress;

        unsigned int parallelism = 3;

        for (unsigned int i = 0; i < population.size(); i++) {
            for (unsigned int s = 0; s < repeats; s++) {
                unsigned int j = rng.getIntBetween(0, population.size() - 2);
                if (j >= i) j++;

                if (matches_in_progress.size() == parallelism) {
                    match m_waitfor = matches_in_progress.front();
                    matches_in_progress.pop();

                    unsigned int score_a = 0, score_b = 0;
                    (*m_waitfor.p) >> score_a >> score_b;

                    assert(score_a + score_b == 1);

                    score[m_waitfor.i] += (2 * score_a) - 1;
                    score[m_waitfor.j] += (2 * score_b) - 1;

                    m_waitfor.p->clear();

                    delete m_waitfor.p;
                    std::cout << "*" << std::flush;
                }

                // pick who plays BLACK randomly
                unsigned int ti, tj;
                if (rng.getBool()) {
                    ti = i; tj = j;
                } else {
                    ti = j; tj = i;
                }

                match m = { createMatch(get(ti), get(tj)), ti, tj };
                matches_in_progress.push(m);
            }
        }

        while (matches_in_progress.size() == parallelism) {
            match m_waitfor = matches_in_progress.front();
            matches_in_progress.pop();

            unsigned int score_a = 0, score_b = 0;
            (*m_waitfor.p) >> score_a >> score_b;

            assert(score_a + score_b == 1);

            score[m_waitfor.i] += score_a;
            score[m_waitfor.j] += score_b;

            m_waitfor.p->clear();

            delete m_waitfor.p;
        }

        while (population.size() > tournament_survivors) {
            unsigned int min = 999999, min_i = 0;
            for (unsigned int i = 0; i < population.size(); i++) {
                if (score[i] < min) {
                    min = score[i];
                    min_i = 0;
                }
            }

            population.at(min_i) = population[population.size() - 1];
            score.at(min_i) = score[population.size() - 1];
            population.pop_back();
            score.pop_back();
        }
    }
};

int main() {
    RNG rng;
    GA_ProblemDefinition_GoParams problem;

    GA_Solver_Go solver(problem);

    unsigned int generations = 50, pop_size = 20;

    std::cout << "Creating initial random population\n";
    solver.addRandomIndividualsUntilSizeIs(pop_size);

    for (unsigned int i = 0; i < generations; i++) {
        std::cout << "Generation " << i << "\n";
        solver.outputCommandLines();

        solver.tournamentSome(10, 1);
        solver.tournamentSome(4, 3);

        solver.outputCommandLines();

        solver.breedSome(8);
        solver.addRandomIndividualsUntilSizeIs(10); // may prevent local minima being reached
        solver.breedSome(pop_size);

        //solver.mutateSome(mutants);
        solver.shufflePopulation();
    }
    solver.tournamentSome(6, 10);
    solver.tournamentSome(3, 10);
    solver.tournamentSome(1, 10); // there can be only 1
    solver.outputCommandLines();

    return 0;
}

