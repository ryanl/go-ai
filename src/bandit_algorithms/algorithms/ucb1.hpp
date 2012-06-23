#include "algorithm.hpp"
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace boost::assign; // bring 'operator+=()' into scope

class BA_UCB1 : public BanditAlgorithm {
private:
    RNG &rng;
    float fpu;
    float exploration_constant;

public:
    BA_UCB1(RNG &_rng) :
        rng(_rng)
    {}

    void setFpu(float _fpu) {
        fpu = _fpu;
    }

    void setExplorationConstant(float _expl) {
        exploration_constant = _expl;
    }

    virtual void changeSettings(const std::vector<float>& vals) {
        if (vals.size() != 2) abort();

        fpu = vals[0];
        exploration_constant = vals[1];
    }

    virtual std::vector<float> getCurrentSettings() {
        std::vector<float> ret;
        ret += fpu, exploration_constant;

        return ret;
    }

    virtual std::vector<Parameter> getDefaultParameters() {
        // value, step, min, max
        Parameter p_fpu = {1.0f, 0.4f, 0.0f, 4.0f};
        Parameter p_expl = {0.7f, 0.2f, 0.0f, 3.0f};

        std::vector<Parameter> ret;
        ret += p_fpu, p_expl;
        return ret;
    }

    virtual unsigned int selectBandit(const std::vector<BanditInfo>& bandits) const {
        const unsigned int total_plays = totalPlays(bandits);

        // play best arm so far
        unsigned int max_i = 0;
        float max_val = -10.0f;

        RandomPermutation rp(rng, bandits.size());

        float log_n = logf(total_plays);

        for (unsigned int k = 0; k < bandits.size(); k++) {
            unsigned int i = rp.getNext();

            float val;
            if (bandits[i].times_played > 0) {
                val = bandits[i].total_reward / float(bandits[i].times_played);

                val += exploration_constant * sqrtf(log_n / bandits[i].times_played);
            } else {
                val = fpu; // first play urgency
            }

            if (val > max_val) {
                max_val = val;
                max_i = i;
            }
        }

        return max_i;
    }
};

