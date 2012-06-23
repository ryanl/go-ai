#include "random/rng.hpp"

class BernoulliBandit {
private:
    float p;
    RNG *rng;

public:
    BernoulliBandit(float _p, RNG &_rng) :
        p(_p),
        rng(&_rng)
    {}

    float getP() const {
        return p;
    }

    bool generateReward() const {
        float r = rng->getFloatIn01();
        assert(r >= 0.0f && r <= 1.0f);
        return (r <= p);
    }
};

