#undef NDEBUG // enable asserts

#include "improved_bitset.hpp"

#include "assert.h"
#include <vector>
#include <set>
#include <iostream>

using namespace std;

bool is_prime(unsigned int n) {
    if (n < 2) return false;

    for (unsigned int i = 2; i * i <= n; i++) {
    if (n % i == 0) return false;
    }
    return true;
}

void test1() {
    ImprovedBitset<12345> a, b;

    /* a and b should start with all bits zero */

    assert(a == b);

    for (unsigned int i = 0; i < 12345; i++) {
        assert(!a.getBit(i));
        assert(!b.getBit(i));
    }

    /* set some of the bits in a non-trivial but deterministic way */
    for (unsigned int i = 0; i < 12345; i++) {
        if (is_prime(i)) {
            a.setBit(i, true);
            assert(a.getBit(i));
        }
    }

    /* check all the ones we did set are set,
       and all the ones we didn't aren't */

    for (unsigned int i = 0; i < 12345; i++) {
        assert((is_prime(i) ^ a.getBit(i)) == 0);
    }

    cout << "a.compare(b) = " << a.compare(b) << endl;

    assert(a != b);
    assert(a > b);
    assert(!(b > a));
    assert(b < a);

    b = a;
    assert(a == b);
    assert(!(a != b));
    assert(a.compare(b) == 0);
    assert(b == a);
}

void test3() {
    {
        ImprovedBitset<123> odds;
        ImprovedBitset<123> evens;

        unsigned c_odds = 0, c_evens = 0;

        for (unsigned int i = 0; i < 123; i++) {
            if (i % 2 == 0) {
                c_evens++;
            } else {
                c_odds++;
            }

            evens[i]  = (i % 2 == 0);
            odds[i] = (i % 2 == 1);
        }

        assert(odds != evens);
        assert(evens != odds);
        assert(~odds == evens);
        assert(odds == ~evens);

        evens = ~odds;
        for (unsigned int i = 0; i < 123; i++) {
            assert(odds[i] == (i % 2 == 1));
            assert(evens[i] == !odds[i]);
        }

        cout << "\nodds.count() == " << odds.count() << " vs. " << c_odds;
        cout << "\nevens.count() == " << evens.count() << " vs. " << c_evens;
        assert(odds.count() == c_odds);
        assert(evens.count() == c_evens);


    }

    cout << "\ntests on bitwise not (~) and count() PASSED\n";
}

template <unsigned int n>
void testSetBitIterator(set<unsigned int> set_values) {
    cout << "Testing SetBitIterator... " << flush;
    {
        ImprovedBitset<n> ib;
        for (set<unsigned int>::iterator it = set_values.begin(); it != set_values.end(); ++it) {
            assert(*it < n);
            ib.setBit(*it);
        }

        typedef typename ImprovedBitset<n>::SetBitIterator it_t;
        it_t sbi = ib.getSetBitIterator();
        for (set<unsigned int>::iterator it = set_values.begin(); it != set_values.end(); ++it) {
            assert(!sbi.isDone());
            assert(ib.getBit(*it));
            cout << "*sbi: " << (*sbi) << ", *it: " << (*it) << "\n";
            if (*sbi != *it) {
                assert(false);
            }
            ++sbi;
        }
        assert(sbi.isDone());
    }

    cout << "PASSED\n";
}


void test2() {
    {
        set<unsigned int> full_1000;
        for (unsigned int i = 0; i < 1000; i++) {
            full_1000.insert(i);
        }
        cout << "testSetBitIterator<1000>(full_1000);\n";
        testSetBitIterator<1000>(full_1000);
        cout << "testSetBitIterator<1001>(full_1000);\n";
        testSetBitIterator<1001>(full_1000);
        cout << "testSetBitIterator<1002>(full_1000);\n";
        testSetBitIterator<1002>(full_1000);
        cout << "testSetBitIterator<1150>(full_1000);\n";
        testSetBitIterator<1150>(full_1000);
    }
    {
        set<unsigned int> empty;
        //cout << "testSetBitIterator<0>(empty);\n";
        //testSetBitIterator<0>(empty);
        cout << "testSetBitIterator<1>(empty);\n";
        testSetBitIterator<1>(empty);
        cout << "testSetBitIterator<2>(empty);\n";
        testSetBitIterator<2>(empty);
        cout << "testSetBitIterator<3>(empty);\n";
        testSetBitIterator<3>(empty);
        cout << "testSetBitIterator<7>(empty);\n";
        testSetBitIterator<7>(empty);
        cout << "testSetBitIterator<63>(empty);\n";
        testSetBitIterator<63>(empty);
        cout << "testSetBitIterator<64>(empty);\n";
        testSetBitIterator<64>(empty);
        cout << "testSetBitIterator<65>(empty);\n";
        testSetBitIterator<65>(empty);
        cout << "testSetBitIterator<128>(empty);\n";
        testSetBitIterator<128>(empty);
        cout << "testSetBitIterator<200>(empty);\n";
        testSetBitIterator<200>(empty);
        cout << "testSetBitIterator<4003>(empty);\n";
        testSetBitIterator<4003>(empty);
    }
    {
        set<unsigned int> odds_500;
        for (unsigned int i = 1; i < 500; i += 2) {
            odds_500.insert(i);
        }
        cout << "testSetBitIterator<500>(odds_500);\n";
        testSetBitIterator<500>(odds_500);
        cout << "testSetBitIterator<501>(odds_500);\n";
        testSetBitIterator<501>(odds_500);
        cout << "testSetBitIterator<1000>(odds_500);\n";
        testSetBitIterator<1000>(odds_500);
    }
    {
        set<unsigned int> primes_100000;
        for (unsigned int i = 1; i < 100000; i += 1) {
            if (is_prime(i))
                primes_100000.insert(i);
        }
        cout << "testSetBitIterator<200000>(primes_100000);\n";
        testSetBitIterator<200000>(primes_100000);
    }
}

int main(int argc, char* argv[]) {
    test1();
    test2();
    test3();

    cout << endl << "PASSED" << endl;

    return 0;
}
