#ifndef __GO_UCT_TREEOPS_HPP
#define __GO_UCT_TREEOPS_HPP

#include <stack>

#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#endif

#include "go_uct.hpp"

class GoUCT;

class GoUCTTeam {

    std::vector<GoUCT*> team_members;

#ifdef USE_BOOST_THREAD
    friend class WorkerFunctor;
    bool please_terminate;
    boost::mutex m;
#endif


    const GoUCTSettings& settings;

public:
    GoUCTTeam(const unsigned int num_members, const GoState& s, const GoUCTSettings& _settings);

    ~GoUCTTeam();

    void ponderFor(unsigned int sleep_ms, unsigned int max_sims = 0);

    GoMove selectMove();

    void resetToNewState(const GoState& s);
    void updateAfterPlay(const GoMove move);
};

#endif
