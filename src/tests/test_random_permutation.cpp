#undef NDEBUG

#include "random/random_permutation.hpp"
#include "random/random_permutation_nonsequential.hpp"
#include "assert.h"
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    RNG rng;
    RNG rng2 = rng;

    for (unsigned int i = 0; i < 50; i++) {
        cout << "Permutation " << i << ": " << flush;

        RandomPermutation              rp(rng, 10);
        RandomPermutationNonSequential rp2(rng2, 10);

        for (unsigned int j = 0; j < 10; j++) {
            assert(!rp.done());
            cout << rp.getNext() << " " << flush;
        }
        assert(rp.done());
        cout << "\n";

        cout << "Permutation (non-sequential way) " << i << ": " << flush;

        for (unsigned int j = 0; j < 10; j++) {
            cout << rp2[j] << " " << flush;
        }
        cout << "\n";
    }

    cout << endl << "EXAMINE OUTPUT MANUALLY" << endl;

    return 0;
}
