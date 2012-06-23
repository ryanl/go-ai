#ifndef __RANDOM_PERMUTATION_NONSEQUENTIAL_HPP
#define __RANDOM_PERMUTATION_NONSEQUENTIAL_HPP

#include "rng.hpp"
#include "assert.h"
#include <vector>

class RandomPermutationNonSequential {
private:
    std::vector<unsigned int> a;
    
public:
    RandomPermutationNonSequential(RNG &rng, unsigned int num_elements) {
        a.resize(num_elements);
        
        for (unsigned int i = 0; i < num_elements; i++) {
            a[i] = i;
        }

        for (unsigned int i = 0; i < num_elements; i++) {
            unsigned int j = rng.getIntBetween(i, a.size() - 1);

            unsigned int tmp = a[j];
            a[j] = a[i];
            a[i] = tmp;
        }
    }
    
    unsigned int operator [] (const unsigned int i) const {
        assert(i < a.size());

        return a[i];
    }
};



#endif
