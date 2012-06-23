#include <vector>
#include "algorithms/algorithm.hpp" // for Parameter

std::vector<float> hillClimb(RNG &rng, std::vector<Parameter> params, ParamFunctional& fn) {
    std::vector<float> p;

    for (unsigned int i = 0; i < params.size(); i++) {
        p.push_back(params[i].value);
    }

    float best_score = -999999.0f;

    unsigned int reps_since_improved = 0;

    float delta = 1.0f;

    while (reps_since_improved < 200) {
        reps_since_improved++;
        std::vector<float> perturbed_p = p;

        bool any_changes = false;
        while (!any_changes) {
            for (unsigned int i = 0; i < params.size(); i++) {
                if (rng.getBool()) {
                    perturbed_p[i] = rng.getFloatIn(params[i].min, params[i].max);
                    any_changes = true;
                }
                /*
                if (rng.getBool()) {
                    float perturbation = rng.getFloatIn01() * (rng.getFloatIn01() + rng.getFloatIn01() - 1.0f) * params[i].step * delta;
                    perturbed_p[i] += perturbation;
                    if (perturbed_p[i] < params[i].min) perturbed_p[i] = params[i].min;
                    if (perturbed_p[i] > params[i].max) perturbed_p[i] = params[i].max;
                }
                */
            }
        }

        float score = fn(perturbed_p);

        if (score > best_score) {
            for (unsigned int i = 0; i < params.size(); i++) {
                std::cout << perturbed_p[i] << " ";
            }

            std::cout << score << " <<<<<<<\n";

            best_score = score;
            delta = 2.0f * delta + 0.2f;
        }
        if (score >= best_score) {
            p = perturbed_p;
        } else {
            delta /= 1.01f;
        }
    }

    return p;
}

