#include "assert.h"

#include "algorithms/algorithm.hpp"
#include "algorithms/ucb1.hpp"
#include "algorithms/ucb1_tuned.hpp"
#include "algorithms/epsilon_n.hpp"
#include "hill_climb.hpp"
#include "graphing.hpp"

#include <fstream>
#include <map>

#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace std;
using namespace boost::assign; // bring 'operator+=()' into scope


int main(int argc, char* argv[]) {
    RNG rng;


    std::cout << "Bandit algorithm comparison program" << std::endl << std::endl;

    /* Setup some bandits */
    std::vector< BernoulliBandit > bandits;

    const float p_values[] = {0.55f, 0.35f, 0.35f, 0.25f, 0.30f, 0.40f, 0.45f, 0.45f, 0.45f, 0.45f, 0.45f};

    for (unsigned int i = 0; i < sizeof(p_values) / sizeof(*p_values); i++) {
        std::cout << "Bandit " << (i + 1) << ": " << p_values[i] << "\n";;
        BernoulliBandit bb(p_values[i], rng);
        bandits.push_back(bb);
    }

    std::cout << "\n";

    BanditProblem bp = { &rng, bandits, 0 };

    vector<AlgorithmSettings> algorithms;

    {
        // we try out UCB1_Tuned from "Finite-time Analysis of the Multiarmed Bandit Problem"

        AlgorithmSettings as = { "UCB1-Tuned", new BA_UCB1_Tuned(rng) };

        std::vector<float> ucb1_tuned_textbook_params;
        ucb1_tuned_textbook_params += 999.0f, 1.0f;
        as.ba->changeSettings(ucb1_tuned_textbook_params);

        algorithms.push_back(as);
    }

    {
        bp.plays = 500;
        PF_BATester_Factory tester_factory(bp);

        // we try out UCB1_Tuned but modified with two parameters - fpu and exploration -
        //  and use hill climbing to (hopefully) find good values for these parameters
        AlgorithmSettings as = { "UCB1-Tuned* hillclimbed 500", new BA_UCB1_Tuned(rng) };
        std::cout << as.algorithm_name << "\n";

        PF_BATester tester = tester_factory.create(as.ba);
        std::vector<float> ucb1_tuned_hc_params = hillClimb(rng, as.ba->getDefaultParameters(), tester);
        as.ba->changeSettings(ucb1_tuned_hc_params);

        algorithms.push_back(as);
    }

    {
        bp.plays = 5000;
        PF_BATester_Factory tester_factory(bp);

        // we try out UCB1_Tuned but modified with two parameters - fpu and exploration -
        //  and use hill climbing to (hopefully) find good values for these parameters
        AlgorithmSettings as = { "UCB1-Tuned* hillclimbed 5000", new BA_UCB1_Tuned(rng) };
        std::cout << as.algorithm_name << "\n";

        PF_BATester tester = tester_factory.create(as.ba);
        std::vector<float> ucb1_tuned_hc_params = hillClimb(rng, as.ba->getDefaultParameters(), tester);
        as.ba->changeSettings(ucb1_tuned_hc_params);

        algorithms.push_back(as);
    }


    {
        AlgorithmSettings as = { "UCB1", new BA_UCB1(rng) };

        // we try the same thing with the original UCB1 and the original parameters
        std::vector<float> ucb1_textbook_params;
        ucb1_textbook_params += 999.0f, 1.0f;
        as.ba->changeSettings(ucb1_textbook_params);

        algorithms.push_back(as);
    }

    {
        bp.plays = 500;
        PF_BATester_Factory tester_factory(bp);
        AlgorithmSettings as = { "UCB1* hillclimbed 500", new BA_UCB1(rng) };
        std::cout << as.algorithm_name << "\n";

        PF_BATester tester = tester_factory.create(as.ba);
        std::vector<float> ucb1_hc_params = hillClimb(rng, as.ba->getDefaultParameters(), tester);
        as.ba->changeSettings(ucb1_hc_params);

        algorithms.push_back(as);
    }

    {
        bp.plays = 5000;
        PF_BATester_Factory tester_factory(bp);
        AlgorithmSettings as = { "UCB1* hillclimbed 5000", new BA_UCB1(rng) };
        std::cout << as.algorithm_name << "\n";

        PF_BATester tester = tester_factory.create(as.ba);
        std::vector<float> ucb1_hc_params = hillClimb(rng, as.ba->getDefaultParameters(), tester);
        as.ba->changeSettings(ucb1_hc_params);

        algorithms.push_back(as);
    }


    {
        bp.plays = 500;
        PF_BATester_Factory tester_factory(bp);
        AlgorithmSettings as = { "\\epsilon_n hillclimbed 500", new BA_EpsilonN_Greedy(rng) };
        std::cout << as.algorithm_name << "\n";

        /* and we also try to hill climb for the lambda parameter on the epsilon_n algorithm
           note: lambda is a function of c and d from the original paper - see implementation */

        PF_BATester tester = tester_factory.create(as.ba);
        std::vector<float> epsilon_n_hc_params = hillClimb(rng, as.ba->getDefaultParameters(), tester);
        as.ba->changeSettings(epsilon_n_hc_params);

        algorithms.push_back(as);
    }

    {
        bp.plays = 5000;
        PF_BATester_Factory tester_factory(bp);

        AlgorithmSettings as = { "\\epsilon_n hillclimbed 5000", new BA_EpsilonN_Greedy(rng) };
        std::cout << as.algorithm_name << "\n";

        /* and we also try to hill climb for the lambda parameter on the epsilon_n algorithm
           note: lambda is a function of c and d from the original paper - see implementation */

        PF_BATester tester = tester_factory.create(as.ba);
        std::vector<float> epsilon_n_hc_params = hillClimb(rng, as.ba->getDefaultParameters(), tester);
        as.ba->changeSettings(epsilon_n_hc_params);

        algorithms.push_back(as);
    }


    bp.plays = 5000;
    AlgorithmGrapher ag;
    LabelledGraph_t graph_data = ag.generateGraphData(bp, algorithms);
    ag.outputGraphForGnuplot(graph_data);
}
