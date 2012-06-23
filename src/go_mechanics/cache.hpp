#ifndef __CACHE_HPP
#define __CACHE_HPP

template <typename T>
class Cache {
    private:
        unsigned int current_iteration;
        std::vector<pair<int, T>> cache;

    public:
        Cache(unsigned int elements) :
            cache(elements, pair<int, T>(0, T()),
            current_iteration(1)
        {

        }

        bool contains(unsigned int key) {
            assert(0 <= key && key < cache.size());

            return cache[key].first == current_iteration;
        }

        T get(unsigned int key) {
            assert(0 <= key && key < cache.size());
            assert(cache[key].first == current_iteration);

            return cache[key].second;
        }

        void expire() {
            current_iteration++;

            if (current_iteration == 0) { // wrap around means you need to invalidate explicitly
                current_iteration = 1;

                for (int i = 0; i < cache.size(); i++) {
                    cache[i].first = 0;
                }
            }
        }
}

#endif __CACHE_HPP
