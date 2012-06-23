#ifndef __RANDOM_PERMUTATION_HPP
#define __RANDOM_PERMUTATION_HPP

#include "rng.hpp"
#include "assert.h"
#include <vector>

/* To do - check if this class' performance is bad */

class RandomPermutation {
private:
    std::vector<unsigned int> a;
    unsigned int outputs_so_far;
    RNG &rng;
    
public:
    RandomPermutation(RNG &_rng, unsigned int num_elements) :
        rng(_rng)
    {
        a.reserve(num_elements);
        for (unsigned int i = 0; i < num_elements; i++) {
            a.push_back(i);
        }
        outputs_so_far = 0;
    }
    
    unsigned int getNext() {
        assert(outputs_so_far < a.size());
        unsigned int i = rng.getIntBetween(outputs_so_far, a.size() - 1);
        unsigned int tmp = a[i];
        a[i] = a[outputs_so_far];
        outputs_so_far++;
        
        return tmp;
    }
    
    bool done() {
        return outputs_so_far == a.size();
    }
};

#endif
