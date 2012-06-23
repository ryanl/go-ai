#undef NDEBUG

#include <iostream>
#include "random/rng.hpp"
#include "assert.h"

using namespace std;

int main(int argc, char* argv[]) {
    RNG r;
    RNG r_copy = r;

    assert(r.getInt() == r_copy.getInt());

    unsigned int reps = 50;

    float f_sum = 0.0f;
    std::cout << "Generating " << reps << " floats in [0,1]\n";

    for (unsigned int i = 0; i < reps; i++) {
        float f = r.getFloatIn01();
        f_sum += f;
        std::cout << r.getFloatIn01() << "\n";
    }

    float f_mean = f_sum / reps;

    std::cout << "\nMean: " << f_mean << "\n";
    if (f_mean < 0.4f || f_mean > 0.6f) {
        assert(false);
    }

    unsigned int count = 0;
    std::cout << "Generating " << reps << " bools\n";
    for (unsigned int i = 0; i < reps; i++) {
        bool b = r.getBool();
        count += b ? 1 : 0;
        std::cout << (b ? "T" : "F");
    }
    float count_proportion = float(count)/reps;
    std::cout << "\n\nCount: " << count << " = " << count_proportion << " proportionally\n";

    if (count_proportion < 0.4f || count_proportion > 0.6f) {
        assert(false);
    }
}
