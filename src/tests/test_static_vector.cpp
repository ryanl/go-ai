#undef NDEBUG
#include "assert.h"
#include "../static_vector.hpp"
#include <vector>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    vector<int> v;
    StaticVector<int, 10> sv;

    int a = 123;

    for (unsigned i = 0; i < 10; i++) {
        v.push_back(a);
        sv.push_back(a);

        a = a * 345;

        assert(v.size() == sv.size());

        for (unsigned int j = 0; j < i; j++) {
            assert(v[j] == sv[j]);
        }
    }

    cout << "PASSED\n";
}
