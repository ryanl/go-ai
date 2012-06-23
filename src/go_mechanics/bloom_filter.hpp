#ifndef __BLOOM_FILTER_HPP
#define __BLOOM_FILTER_HPP

template <typename T, typename H>
class Bloomfilter {
    private:
        unsigned int hash_sources[2];
        vector<bool> table;

    public:
        bool check(const T& x) {
            for (int i = 0; i < 2; i++) {
                unsigned int h = H.hash(x, hash_sources[i]) % table.size();
                if (!table[h]) return false;
            }
            return true;
        }

        void add(const T& x) {
            for (int i = 0; i < 2; i++) {
                unsigned int h = H.hash(x, hash_sources[i]) % table.size();
                table[h] = true;
            }
        }

        Bloomfilter(unsigned int entries)  : table(entries, false) {
            /* 'randomly' picked by me */
            hash_sources[0] = (unsigned int) 0x5223498023408422LL;
            hash_sources[1] = (unsigned int) 0x9040920934239041LL;
        }
}




#endif
