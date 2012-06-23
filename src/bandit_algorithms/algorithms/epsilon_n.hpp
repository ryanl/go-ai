#include "algorithm.hpp"
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace boost::assign; // bring 'operator+=()' into scope

class BA_EpsilonN_Greedy : public BanditAlgorithm {
private:
    float lambda;
    RNG &rng;

public:
    // notes - lambda = c / d^2 where c and d are as in papers on the topic
    BA_EpsilonN_Greedy(RNG &_rng) :
        rng(_rng)
    {
    }

    BA_EpsilonN_Greedy(RNG &_rng, float _lambda) :
        lambda(_lambda),
        rng(_rng)
    {}


    void setLambda(float _lambda) {
        lambda = _lambda;
    }


    virtual void changeSettings(const std::vector<float>& vals) {
        if (vals.size() != 1) abort();

        lambda = vals[0];
    }

    virtual std::vector<Parameter> getDefaultParameters() {
        // value, step, min, max
        Parameter p_lambda = {20.0f, 20.0f, 0.0f, 500.0f};

        std::vector<Parameter> ret;
        ret += p_lambda;

        return ret;
    }

    virtual std::vector<float> getCurrentSettings() {
        std::vector<float> ret;
        ret += lambda;

        return ret;
    }

    virtual unsigned int selectBandit(const std::vector<BanditInfo>& bandits) const {
        const unsigned int total_plays = totalPlays(bandits);

        float epsilon_n = (lambda / (total_plays + 1.0f));


        //if (epsilon_n > 1.0f) epsilon_n = 1.0f;

        const float r = rng.getFloatIn01(); // in closed interval [0,1]

        assert(r >= 0.0f && r <= 1.0f);

        if (r <= epsilon_n) {
            // play a random arm

            return rng.getIntBetween(0, bandits.size() - 1);
        } else {

            // play best arm so far
            unsigned int max_i = 0;
            float max_mean = -1.0f;

            RandomPermutation rp(rng, bandits.size());

            for (unsigned int k = 0; k < bandits.size(); k++) {
                unsigned int i = rp.getNext();

                float mean;
                if (bandits[i].times_played > 0) {
                    mean = bandits[i].total_reward / float(bandits[i].times_played);
                } else {
                    mean = 0.0f;
                }

                if (mean > max_mean) {
                    max_mean = mean;
                    max_i = i;
                }
            }

            return max_i;
        }
    }
};
