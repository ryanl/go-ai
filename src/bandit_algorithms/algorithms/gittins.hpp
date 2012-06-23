#include "algorithm.hpp"
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace boost::assign; // bring 'operator+=()' into scope

class BA_Gittins : public BanditAlgorithm {
private:

    float factorial(unsigned int a) {
        float ret = 1;
        for (unsigned int i = 2; i < a; i++) {
            ret *= i;
        }
        return ret;
    }

    void estimateGittinsIndex(unsigned int wins, unsigned int losses, float discount)
        // 1000 is arbitrary

        float max_val = -1.0f;
        float sum = 0.0f, apower = discount;

        // not sure if this is right

        //float beta_scale = factorial(wins + losses + 1) / (factorial(wins + 1) * factorial(losses + 1))

        float expectation = (wins + 1.0f) / (wins + losses + 2.0f);
        float perm = a_fac * b_fac / ab_fac;

        for (unsigned int t = 0; t < 1000; t++) {
            sum += apower;
            apower *= discount;

            float gittins = (expectation * / sum;
        }
    }

public:
    BA_Gittins() {}

    virtual void changeSettings(const std::vector<float>& vals) {
        if (vals.size() != 0) abort();
    }

    virtual std::vector<float> getCurrentSettings() {
        std::vector<float> ret; // empty
        return ret;
    }

    virtual std::vector<Parameter> getDefaultParameters() {
        std::vector<Parameter> ret; // empty
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

