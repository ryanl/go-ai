#include <string>
#include <vector>
#include <algorithm>
#include <boost/variant.hpp>
#include "random/rng.hpp"
#include "assert.h"

typedef boost::variant< int, float, std::string > GeneValue_t;


enum GA_GameWinner {
    GA_WIN_A,
    GA_WIN_B,
    GA_DRAW
};

class GA_GeneType {
private:
    std::string param_name;

public:
    GA_GeneType(const std::string& _param_name) : param_name(_param_name) {}

    const std::string& getParameterName() const {
        return param_name;
    }

    virtual GeneValue_t getRandomValue(RNG &rng) const = 0;
    virtual GeneValue_t combine(RNG &rng, const GeneValue_t& a, const GeneValue_t& b) const = 0;
};


class GA_GeneTypeTrivial : public GA_GeneType {

public:
    GA_GeneTypeTrivial(const std::string& _enum_name) : GA_GeneType(_enum_name) {}

    GeneValue_t getRandomValue(RNG &rng) const {
        return 0;
    }

    GeneValue_t combine(RNG &rng, const GeneValue_t& a, const GeneValue_t& b) const {
        return 0;
    }
};

class GA_GeneTypeEnum : public GA_GeneType {
public:
    std::vector<std::string> possible_values;

    GA_GeneTypeEnum(const std::string& _enum_name) : GA_GeneType(_enum_name) {}

    void addPossibleValue(const std::string& s) {
        possible_values.push_back(s);
    }

    GeneValue_t getRandomValue(RNG &rng) const {
        return possible_values.at(rng.getIntBetween(0, possible_values.size() - 1));
    }

    GeneValue_t combine(RNG &rng, const GeneValue_t& a, const GeneValue_t& b) const {
        if (rng.getBool()) {
            return a;
        } else {
            return b;
        }
    }
};

class GA_GeneTypeBoundedFloat : public GA_GeneType {
private:
    float min_f, max_f;
    float range_expansion_bonus;
    float range_expansion_coefficient;

public:
    GA_GeneTypeBoundedFloat(const std::string& _enum_name, float _min_f, float _max_f, float _range_expansion_bonus, float _range_expansion_coefficient) :
        GA_GeneType(_enum_name),
        min_f(_min_f),
        max_f(_max_f),
        range_expansion_bonus(_range_expansion_bonus),
        range_expansion_coefficient(_range_expansion_coefficient)
    {}

    GeneValue_t getRandomValue(RNG &rng) const {
        return rng.getFloatIn(min_f, max_f);
    }

    GeneValue_t combine(RNG &rng, const GeneValue_t& a, const GeneValue_t& b) const {
        float fa = boost::get<float>(a), fb = boost::get<float>(b);
        float range_min = std::min(fa, fb), range_max = std::max(fa, fb);
        float range_size = range_max - range_min;
        float expand = (range_expansion_coefficient * range_size) + range_expansion_bonus;

        range_min -= expand;
        range_max += expand;

        if (range_min < min_f) range_min = min_f;
        if (range_max > max_f) range_max = max_f;

        float f = rng.getFloatIn(range_min, range_max);
        return GeneValue_t(f);
    }
};


class GA_ProblemDefinition {
public:
    virtual const std::vector<GA_GeneType*>& getGeneTypes() const = 0;
    virtual GA_GameWinner playGameBetween(const std::vector<GeneValue_t>& a, const std::vector<GeneValue_t>& b) = 0;
    virtual GA_GameWinner playGameAgainstPredator(const std::vector<GeneValue_t>& a) = 0;
};

class GA_Solver {
public:
    RNG rng;

    GA_ProblemDefinition &pd;

    const std::vector<GA_GeneType*>& gene_types;


    std::vector<GeneValue_t> generateRandomIndividual() {
        std::vector<GeneValue_t> ret;
        ret.reserve(gene_types.size());

        for (unsigned int i = 0; i < gene_types.size(); i++) {
            ret.push_back(gene_types[i]->getRandomValue(rng));
        }

        return ret;
    }

    std::vector< std::vector<GeneValue_t> > population;


    std::vector<GeneValue_t>& get(unsigned int i) {
        return population.at(i);
    }

    GA_Solver(GA_ProblemDefinition &_pd) :
        pd(_pd),
        gene_types(pd.getGeneTypes())
    {}

    virtual void addRandomIndividualsUntilSizeIs(unsigned int population_size) {
        while (population.size() < population_size) {
            population.push_back(generateRandomIndividual());
        }
    }

    virtual void predateSome(unsigned int min_survivors) {
        std::vector<unsigned int> losses(population.size(), 0);

        unsigned int total_wins = 0, total_losses = 0;

        for (unsigned int i = 0; i < population.size(); i++) {
            for (unsigned int r = 0; r < 1000; r++) {
                // attack individual non-randomly (already should be shuffled)
                GA_GameWinner game_result = pd.playGameAgainstPredator(population[i]);

                if (game_result == GA_WIN_B) {
                    losses[i]++;
                    total_losses++;
                } else {
                    total_wins++;
                }
            }
            //std::cout << "losses[" << i << "] = " << losses[i] << "\n";
        }

        std::vector< std::vector<GeneValue_t> > new_population;

        unsigned int threshold = 0, i = 0;
        while (new_population.size() < min_survivors) {
            if (losses[i] == threshold) {
                if (new_population.size() == 0) {
                    std::cout << (1000 - threshold) << " ";
                }
                new_population.push_back(population[i]);
            }

            i = (i + 1) % population.size();
            if (i == 0) threshold++;
        }



        population = new_population;
    }

    virtual void fightSome(unsigned int survivors, unsigned int start_hp) {
        std::vector<int> hp(population.size(), start_hp * 2);

        unsigned int max_trials = population.size() * start_hp * 3; // arbitrary

        for (unsigned int trial = 0; trial < max_trials && population.size() > survivors; trial++) {
            // choose two distinct individuals

            unsigned int a = rng.getIntBetween(0, population.size() - 1);
            unsigned int b = rng.getIntBetween(0, population.size() - 2);
            if (b >= a) b++;

            GA_GameWinner game_result = pd.playGameBetween(population[a], population[b]);

            int loser = -1, winner = -1;
            if (game_result == GA_WIN_A) {
                loser = b;
                winner = a;
            } else if (game_result == GA_WIN_B) {
                loser = a;
                winner = b;
            }

            if (loser != -1) {
                hp.at(winner)++; // compensate winner for the bad luck of being chosen to fight
                hp.at(loser) -= 2;
                if (hp.at(loser) <= 0) {
                    // kill of the loser if it has <= 0 HP
                    population.at(loser) = population[population.size() - 1];
                    hp.at(loser) = hp[population.size() - 1];

                    population.pop_back();
                    hp.pop_back();
                }
            }
        }

        // if we didn't get enough victims then we got a lot of draws so we will pick off some unlucky few
        while (population.size() > survivors) population.pop_back();
    }

    virtual std::vector<GeneValue_t> breed(const std::vector<GeneValue_t>& a, const std::vector<GeneValue_t>& b) {
        std::vector<GeneValue_t> ret;
        ret.reserve(a.size());

        assert(a.size() == b.size());
        assert(a.size() == gene_types.size());

        for (unsigned int i = 0; i < a.size(); i++) {
            GeneValue_t new_gene = gene_types.at(i)->combine(rng, a[i], b[i]);
            ret.push_back(new_gene);
        }

        return ret;
    }

    void shufflePopulation() {
        shuffle(population);
    }

    template <typename T>
    void shuffle(std::vector<T> &v) {
        for (unsigned int i = 0; i < v.size(); i++) {
            unsigned int j = rng.getIntBetween(i, v.size() - 1);
            swap(v[i], v[j]);
        }
    }

    virtual void breedSome(unsigned int new_population_size) {
        unsigned int breeders = population.size();

        while (population.size() < new_population_size) {
            // choose two distinct individuals

            unsigned int a = rng.getIntBetween(0, breeders - 1);
            //population.push_back(population[a]);
            unsigned int b = rng.getIntBetween(0, breeders - 1);

            population.push_back(breed(population[a], population[b]));
            //std::cout << "b";
        }
     }

    virtual void breedSomeKillParents(unsigned int new_population_size) {
        unsigned int breeders = population.size();
        std::vector< std::vector<GeneValue_t> > new_population;

        while (new_population.size() < new_population_size) {
            // choose two distinct individuals

            unsigned int a = rng.getIntBetween(0, breeders - 1);
            unsigned int b = rng.getIntBetween(0, breeders - 1);

            new_population.push_back(breed(population[a], population[b]));
            //std::cout << "b";
        }
        swap(population, new_population);
     }


     virtual void add(std::vector<GeneValue_t> x) {
        population.push_back(x);
     }

     virtual void mutateSome(unsigned int mutants) {
        shuffle(population);
        if (mutants > population.size()) {
            assert(false);
            abort();
        }

        for (unsigned int i = 0; i < mutants; i++) {
            unsigned int mutations = rng.getIntBetween(0, gene_types.size() / 20);

            for (unsigned int j = 0; j < mutations; j++) {
                unsigned int r = rng.getIntBetween(0, gene_types.size() - 1);
                population[i].at(r) = gene_types[r]->getRandomValue(rng);
            }
        }
    }
};
