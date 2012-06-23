#ifndef __IMPROVED_BITSET_HPP
#define __IMPROVED_BITSET_HPP

#include "assert.h"
#include "stdint.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <algorithm>

template <unsigned long bit_count>

class ImprovedBitset {
public:
    typedef uint64_t WordType;

    // 0 = highest bit, 31 = lowest bit
    // branchy code
    /*
    static inline uint32_t getIndexHighestSetBit_uint32(uint32_t v) {
        const uint32_t b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
        const uint32_t S[] = {1, 2, 4, 8, 16};

        register uint32_t r = 0; // result of log2(v) will go here
        for (int i = 4; i >= 0; i--) { // unroll for speed...
            if (v & b[i]) {
                v >>= S[i];
                r |= S[i];
            }
          }
          return 31 - r;
    }
    */

    static inline uint64_t getIndexHighestSetBit_uint64(uint64_t v) {
        const uint64_t b[] = {0x2ULL, 0xCULL, 0xF0ULL, 0xFF00ULL, 0xFFFF0000ULL, 0xFFFFFFFF00000000ULL};
        const uint64_t S[] = {1, 2, 4, 8, 16, 32};

        uint64_t r = 0; // result of log2(v) will go here
        for (int i = 5; i >= 0; i--) { // unroll for speed...
            if (v & b[i]) {
                v >>= S[i];
                r |= S[i];
            }
          }
          return 63 - r;
    }

    // branchless code
    /*
    static inline uint32_t getIndexHighestSetBit_uint32(uint32_t v) {
        uint32_t shift, offset;
        offset =     (v > 0xFFFF) << 4; v >>= offset;
        shift = (v > 0xFF  ) << 3; v >>= shift; offset |= shift;
        shift = (v > 0xF   ) << 2; v >>= shift; offset |= shift;
        shift = (v > 0x3   ) << 1; v >>= shift; offset |= shift;
        offset |= (v >> 1);
        return 31 - offset;
    }
    */

    static const unsigned int  word_size = 64; // bits in a word
    static const unsigned long words = ((bit_count - 1UL) / word_size) + 1UL;
    static const WordType      word_mask = word_size - 1UL;

    class reference {
        friend class ImprovedBitset;
        private:
            ImprovedBitset &parent;
            unsigned int index;
            reference(ImprovedBitset& _parent, unsigned int _index) :
                parent(_parent),
                index(_index)
            {}

        public:
            // set
            reference& operator = (bool x) {
                parent.setBit(index, x);
                return *this;
            }

            // get
            operator bool() const {
                return parent.getBit(index);
            }
    };


    class SetBitIterator {
        friend class ImprovedBitset;

        private:
            const ImprovedBitset<bit_count>* parent;
            unsigned int last_offset_checked;
            WordType mask;
            unsigned int word_index;

            SetBitIterator(const ImprovedBitset<bit_count>* _parent) :
                parent(_parent),
                last_offset_checked((unsigned int)-1),
                mask(-1L),
                word_index(0)
            {
                ++(*this);
            }

        public:
            bool isDone() const {
                return word_index == parent->words;
            }
            unsigned int operator * () const {
                return (word_index * parent->word_size) + last_offset_checked;
            }
            void operator ++ () {
                assert(word_index != parent->words);

                for (; word_index < words; ++word_index) {
                    WordType masked_word = parent->storage[word_index] & mask;

                    if (masked_word) {
#ifdef HAS_BUILTIN_CLZ
                        last_offset_checked = __builtin_clzll(masked_word);
#else
                        last_offset_checked = getIndexHighestSetBit_uint64(masked_word);
#endif

                        mask = ((WordType)-1L) >> last_offset_checked;
                        mask >>= 1; // have to do separately as >> 64 has no effect

                        return;
                    }
                    mask = (WordType)-1L;
                    last_offset_checked = (unsigned int)-1;
                }
                // no more bits
            }
    };

    SetBitIterator getSetBitIterator() const {
        return SetBitIterator(this);
    }
    int getFirst() const {
        SetBitIterator sbi = getSetBitIterator();
        return *sbi;
    }

    reference operator [] (unsigned int index) {
        return reference(*this, index);
    }


    unsigned int size() const {
        return bit_count;
    }

    ImprovedBitset() {
        for (unsigned int i = 0; i < words; i++) {
            storage[i] = 0L;
        }
    }

    bool any() const {
        for (unsigned long i = 0; i < words; i++) {
            if (storage[i]) return true;
        }
        return false;
    }

    int getBit(unsigned int position) const {
        assert(position < bit_count);

        unsigned long word_number = position / word_size;
        WordType bit_in_word = position & word_mask;
        WordType shift       = word_mask - bit_in_word;
        WordType one         = 1;

        return (storage[word_number] >> shift) & one;
    }

    void setBit(unsigned int position, bool value) {
        assert(position < bit_count);

        unsigned long word_number  = position / word_size;
        WordType bit_in_word  = position & word_mask;
        WordType shift = (word_size - 1) - bit_in_word;
        const WordType one = 1;
        WordType bit_mask  = one << shift;

        storage[word_number] = (storage[word_number] & ~bit_mask) | ((value & one) << shift);
    }

    void setBit(unsigned int position) {
        assert(position < bit_count);

        unsigned int word_number  = position / word_size;
        WordType bit_in_word  = position & word_mask;
        WordType shift = (word_size - 1) - bit_in_word;
        WordType set_bit = ((WordType)1) << shift;

        storage[word_number] = storage[word_number] | set_bit;
    }

    void unsetBit(unsigned int position) {
        assert(position < bit_count);

        unsigned int word_number  = position / word_size;

        WordType bit_in_word  = position & word_mask;
        WordType shift = (word_size - 1) - bit_in_word;
        WordType bit_mask  = ((WordType)1) << shift;

        storage[word_number] = (storage[word_number] & ~bit_mask);
    }

    int compare(const ImprovedBitset& other) const {
        for (unsigned int i = 0; i < words; i++) {
            if (storage[i] > other.storage[i])      return 1;
            else if (storage[i] < other.storage[i]) return -1;
        }
        return 0;
    }

    bool operator > (const ImprovedBitset& other) const {
        return compare(other) == 1;
    }

    bool operator < (const ImprovedBitset<bit_count>& other) const {
        return compare(other) == -1;
    }

    bool operator != (const ImprovedBitset<bit_count>& other) const {
        return !(*this == other);
    }

    bool operator == (const ImprovedBitset<bit_count>& other) const {
        for (unsigned int i = 0; i < words; i++) {
            if (storage[i] != other.storage[i]) {
                return false;
            }
        }
        return true;
    }


    ImprovedBitset<bit_count>& operator |= (const ImprovedBitset<bit_count>& other) {
        for (unsigned int i = 0; i < words; i++) {
            storage[i] |= other.storage[i];
        }
        return *this;
    }

    ImprovedBitset<bit_count>& operator &= (const ImprovedBitset<bit_count>& other) {
        for (unsigned int i = 0; i < words; i++) {
            storage[i] &= other.storage[i];
        }
        return *this;
    }

    ImprovedBitset<bit_count>& operator ^= (const ImprovedBitset<bit_count>& other) {
        for (unsigned int i = 0; i < words; i++) {
            storage[i] ^= other.storage[i];
        }
        return *this;
    }

    ImprovedBitset<bit_count> operator ^ (const ImprovedBitset<bit_count>& other) const {
        ImprovedBitset<bit_count> ret = *this;
        ret ^= other;
        return ret;
    }

    ImprovedBitset<bit_count> operator | (const ImprovedBitset<bit_count>& other) const {
        ImprovedBitset<bit_count> ret = *this;
        ret |= other;
        return ret;
    }

    void zero() {
        for (unsigned int i = 0; i < words; i++) {
            storage[i] = 0L;
        }
    }


    void flip() {
        for (unsigned int i = 0; i < words; i++) {
            storage[i] = ~storage[i];
        }
        if (bit_count % word_size != 0) {
            // sort out final word
            WordType final_word_mask = (WordType)-1L;
            final_word_mask >>= (bit_count % word_size);
            storage[words - 1] ^= final_word_mask;
        }
    }

    ImprovedBitset<bit_count> operator ~ () const {
        ImprovedBitset ret;
        for (unsigned int i = 0; i < words; i++) {
            ret.storage[i] = ~storage[i];
        }
        if (bit_count % word_size != 0) {
            // sort out final word
            WordType final_word_mask = (WordType)-1L;
            final_word_mask >>= (bit_count % word_size);
            ret.storage[words - 1] ^= final_word_mask;
        }
        return ret;
    }

    ImprovedBitset<bit_count> operator & (const ImprovedBitset<bit_count>& other) const {
        ImprovedBitset<bit_count> ret = *this;
        ret &= other;
        return ret;
    }

    /* for hashmap etc. */
    unsigned long hash(unsigned long seed = 0) const {
        unsigned long ret = seed;

        for (unsigned long i = 0; i < words; i++) {
            ret ^= storage[i] - (ret >> 3);
            ret += (storage[i] << 32)  ^ (storage[i] >> 32);
        }
        return ret;
    }

    std::string toString() const {
        std::string ret;
        for (unsigned int i = 0; i < bit_count; i++) {
            ret.push_back('0' + getBit(i));
        }
        return ret;
    }

    /*
    unsigned int count() {
        unsigned int ret = 0;

        for (unsigned int i = 0; i < words; i++) {
            uint32_t v = storage[i];

            // from AMD's Software Optimization Guide for AMD64 Processors

            v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
            v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
            v = (v + (v >> 4)) & 0xF0F0F0F;
            v *= 0x01010101;
            v >>= 24; // count
            ret += v;
        }

        return ret;
    }*/

    unsigned int count() const {
#ifdef USE_BUILTIN_POPCOUNT
        unsigned int accumulator = 0;
        for (unsigned int i = 0; i < words; i++) {
            accumulator += __builtin_popcountll(storage[i]); //__builtin_popcountl(storage[i]);
        }
        return accumulator;
#else
        const uint64_t m1  = 0x5555555555555555ULL; //binary: 0101...
        const uint64_t m2  = 0x3333333333333333ULL; //binary: 00110011..
        const uint64_t m4  = 0x0f0f0f0f0f0f0f0fULL; //binary:  4 zeros,  4 ones ...
        const uint64_t h01 = 0x0101010101010101ULL; //the sum of 256 to the power of 0,1,2,3...

        unsigned int ret   = 0;

        for (unsigned int i = 0; i < words; i++) {
            uint64_t x = storage[i];
            x -= (x >> 1) & m1;             // put count of each 2 bits into those 2 bits
            x = (x & m2) + ((x >> 2) & m2); // put count of each 4 bits into those 4 bits
            x = (x + (x >> 4)) & m4;        // put count of each 8 bits into those 8 bits
            ret += (x * h01) >> 56;         // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
        }

        return ret;
#endif
    }


private:
    WordType storage[words];
};

template <unsigned int n>
class ImprovedBitsetHasher {
public:
    size_t operator () (const ImprovedBitset<n>& ib) const { return ib.hash(); }
};

#endif

