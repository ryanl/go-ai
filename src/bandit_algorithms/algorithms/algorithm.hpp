#ifndef __ALGORITHM_HPP
#define __ALGORITHM_HPP

#include <iostream>
#include <vector>
#include <cmath>

#include "random/random_permutation.hpp"

struct BanditInfo {
    unsigned int times_played;
    unsigned int total_reward;
};

class ParamFunctional {
public:
    virtual float operator () (std::vector<float> params) = 0;
};

struct Parameter {
    float value, step, min, max;
};

class BanditAlgorithm {
    public:
        virtual unsigned int selectBandit(const std::vector<BanditInfo>& bandits) const = 0;
        virtual void changeSettings(const std::vector<float>& vals) = 0;
        virtual std::vector<float> getCurrentSettings() = 0;
        virtual std::vector<Parameter> getDefaultParameters() = 0;
};

inline unsigned int totalPlays(const std::vector<BanditInfo>& v) {
    unsigned int ret = 0;

    for (unsigned int i = 0; i < v.size(); i++) {
        ret += v[i].times_played;
    }
    return ret;
}

#include "../bandits.hpp"

struct BanditProblem {
    RNG *rng;
    std::vector<BernoulliBandit> bandits;
    unsigned int plays;
};

inline std::vector<unsigned int> testBanditAlgorithm(BanditAlgorithm &ba, const BanditProblem& bp) {
    std::vector<unsigned int> readings;
    readings.reserve(bp.plays);

    std::vector<BanditInfo> history;

    for (unsigned int j = 0; j < bp.bandits.size(); j++) {
        BanditInfo bi = {0, 0};
        history.push_back(bi);
    }

    unsigned int total_reward = 0;

    for (unsigned int i = 0; i < bp.plays; i++) {
        readings.push_back(total_reward);

        unsigned int choice = ba.selectBandit(history); // have the algorithm choose an arm

        assert(choice < bp.bandits.size());

        bool reward = bp.bandits[choice].generateReward(); // play it

        history[choice].times_played++;
        total_reward += reward ? 1 : 0;
        history[choice].total_reward += reward ? 1 : 0;
    }

    return readings;
}


/* ParamFunctionals are used for hill-climbing */

struct PF_BATester : public ParamFunctional {
    BanditProblem bp;
    BanditAlgorithm *ba;

    PF_BATester(BanditProblem &_bp, BanditAlgorithm *_ba) :
        bp(_bp),
        ba(_ba)
    {}

    float operator () (std::vector<float> p) {
        ba->changeSettings(p);

        // increase trials to reduce error (variance) of readings but increase running time
        const unsigned int trials = 1000;

        unsigned int total = 0;
        for (unsigned int i = 0; i < trials; i++) {
            std::vector<unsigned int> readings = testBanditAlgorithm(*ba, bp);
            total += readings[readings.size() - 1];
        }
        //std::cout << "." << std::flush;

        return total / float(trials * bp.plays);
    }
};

class PF_BATester_Factory {
private:
    BanditProblem bp;

public:
    PF_BATester_Factory(BanditProblem _bp) :
        bp(_bp)
    {}

    PF_BATester create(BanditAlgorithm *ba) {
        return PF_BATester(bp, ba);
    }
};

#endif
