#pragma once

#include <valarray>

#include "generic.hpp"

namespace toy_project {
    struct RealVectorFormat {
        using Type = float;
        using Args = unsigned int;
#ifdef __AVX512F__
        // 512 bit vectors
        const static unsigned int ALIGNMENT = 512/8;
#else
        // 256 bit vectors
        const static unsigned int ALIGNMENT = 256/8;
#endif

        static unsigned int storage_dimensions(Args dimensions) {
            return dimensions;
        }

        static void store(
            const std::valarray<float>& input,
            Type* storage,
            DatasetDescription<RealVectorFormat> dataset
        ) {
            if (input.size() != dataset.args) {
                throw std::invalid_argument("input.size()");
            }
            for (size_t i=0; i < dataset.args; i++) {
                storage[i] = input[i];
            }
            for (size_t i=dataset.args; i < dataset.storage_len; i++) {
                storage[i] = 0.0;
            }
        }

        static void free(Type&) {}

        static std::valarray<float> generate_random(unsigned int dimensions) {
            std::normal_distribution<float> normal_distribution(0.0, 1.0);
            auto& generator = get_default_random_generator();
            std::valarray<float> values(dimensions);
            for (unsigned int i=0; i<dimensions; i++) {
                values[i] = normal_distribution(generator);
            }
            return values;
        }
    };
}
