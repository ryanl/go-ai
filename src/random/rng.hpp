#ifndef __RANDOM_HPP
#define __RANDOM_HPP

#include <iostream>
#include <time.h>
#include <cstdlib>
#include <cstdio>
#include <limits.h>
#include <bitset>

#define numbits(x) (sizeof(x) * 8)

/*!
@class RNG

@brief This is a light-weight random number generator.

It uses a 64-bit LCG to generate 'randomness'. It is fast but not a
strong RNG (don't use it for cryptography).
Objects of this class contain the state of the random number generator.

@author Ryan Lothian
*/

const unsigned long long _a = 0x456789012345679LL;
const unsigned long long _c = 0x126543217654321LL;

class RNG
{
private:
    /*!
        multiplier of the LCG
    */
    static const unsigned long long a = (_a << 32ULL) ^ _a;

    /*!
        increment of the LCG
    */
    static const unsigned long long c = ((_c + _a) << 32ULL) ^ _c;

    unsigned long long state;

    /*!
        Generates the next LCG state by
        state = (state * a) + c
    */
    void iterate_lcg() {
        state *= a;
        state += c;
    }

public:

    void merge(const RNG &_rng) {
        state = state ^ _rng.state;
    }

    /*!
    Experimental.
    Allows several fairly uncorrelated RNGs to be produced from one RNG.
    */
    void branch(unsigned int thread_id) {
        state ^= thread_id    * 0x79292529293LL;
        state += 0x123123123123LL;
        state *= -thread_id;

        for (unsigned int i = 0; i <= thread_id; i++) {
            state ^= 1 << (i * 2);
            iterate_lcg();
        }
    }

    void gainEntropy() {
#ifdef SEED_RNG
        FILE *fh = fopen("/dev/urandom", "rb");
        unsigned int modifier = 0;
        for (unsigned int i = 0; i < sizeof(unsigned long long) / 8; i++) {
            int c = fgetc(fh);
            if (c == EOF) abort();
            modifier = (modifier ^ c) << 8;
        }
        state ^= modifier;
        fclose(fh);

        // if /dev/urandom is not available you could use unsigned long long)time(NULL);
#endif
    }

    RNG() {
        state = 123456789123LL;
        gainEntropy();
        iterate_lcg();
    }

    /*!
    Generates a random integer in the closed interval [min, max].
    */
    int getIntBetween(int min, int max) {
        iterate_lcg();

        // split the high transform over two parts so we can
        // do multiplication without overflow
        int half_bit_shift = numbits(long long) >> 1;

        unsigned long long range = 1 + max - min;

        return min + int(((state >> half_bit_shift) * range) >> half_bit_shift);

    }

    unsigned int getInt()  {
        iterate_lcg();

        // high transform
        unsigned long long shift = numbits(long long) - numbits(int);
        return state >> shift;
    }

    /*!
    Generates a random float in the closed interval [0, 1]
    */
    float getFloatIn01() {
        iterate_lcg();

        float ret = float(getInt()) / float((unsigned int)(-1));

        return ret;
    }

    /*!
    Generates a random float in the closed interval [min, max]
    */
    float getFloatIn(float min, float max) {
        float ret = getFloatIn01();
        ret *= (max - min);
        ret += min;

        return ret;
    }

    /*!
    Generates a random bit.
    */
    bool getBool() {
        iterate_lcg();

        // use the top bit
        return state >> (numbits(long long) - 1ULL);
    }

    /*!
    Generates n random bits.
    */
    template<int n> std::bitset<n> getBits() {
        std::bitset<n> ret;

        for (unsigned int i = 0; i < n; i++) {
            ret[i] = getBool();
        }
        return ret;
    }
};

#endif

