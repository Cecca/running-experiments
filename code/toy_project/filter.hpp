#pragma once

#include "typedefs.hpp"
#include "generic.hpp"
#include "distance.hpp"

namespace toy_project {

    template <typename T>
    class SimHashFunction {
        AlignedStorage<T> hash_vec;
        unsigned int dimensions;

    public:
        SimHashFunction(DatasetDescription<T> dataset, std::mt19937_64 & generator)
          : hash_vec(allocate_storage<T>(1, dataset.storage_len)),
            dimensions(dataset.storage_len)
        {
            auto vec = T::generate_random(dataset.args, generator);
            T::store(vec, hash_vec.get(), dataset);
        }

        LshDatatype operator()(typename T::Type* vec) const {
            auto dot = dot_product_i16_simple(hash_vec.get(), vec, dimensions);
            return dot >= 0.0f;
        }
    };

    template <>
    LshDatatype SimHashFunction<RealVectorFormat>::operator()(float* vec) const {
            auto dot = dot_product_simple(hash_vec.get(), vec, dimensions);
            return dot >= 0.0f;
    }

    template <typename T>
    class Filter {
    private:
        uint_fast8_t max_sketch_diff;
        std::vector<SimHashFunction<T>> hash_vecs;
        std::vector<uint64_t> query_sketches;
        std::vector<uint64_t> data_sketches;
        float recall;

    public:
        Filter(float recall):recall(recall) { 
            reset();
        }

        void reset() {
            max_sketch_diff = NUM_FILTER_HASHBITS;
        }

        void setup(const VectorStorage<T>& data,
                const VectorStorage<T>& queries,
                uint64_t seed) {
            std::mt19937_64 generator(seed);

            for (size_t i = 0; i < NUM_FILTER_HASHBITS; i++) {
                hash_vecs.push_back(SimHashFunction<T>(data.get_description(), generator));
            }

            for (size_t i = 0; i < data.get_size(); i++) {
                uint64_t sketch = 0;
                for (size_t j = 0; j < NUM_FILTER_HASHBITS; j++) {
                    sketch = sketch << 1;
                    sketch += hash_vecs[j](data[i]);
                }
                data_sketches.push_back(sketch);
            }

            for (size_t i = 0; i < queries.get_size(); i++) {
                uint64_t sketch = 0;
                for (size_t j = 0; j < NUM_FILTER_HASHBITS; j++) {
                    sketch <<= 1;
                    sketch += hash_vecs[j](queries[i]);
                }
                query_sketches.push_back(sketch);
            }
        }

        // Check if the value at position idx in the dataset passes the next filter.
        // A value can only pass one filter.
        inline bool passes(size_t query_index, size_t dataset_index ) const {
            uint_fast8_t sketch_diff = popcountll(
                    query_sketches[query_index] ^ data_sketches[dataset_index]);
            return (sketch_diff <= max_sketch_diff);
        }

        inline void update(float d) {
            auto exp = NUM_FILTER_HASHBITS * (std::acos(d) / M_PI);
            auto diff = std::sqrt(exp * std::log(1/(1-recall)));
            max_sketch_diff = std::roundf(exp + diff);
        }
    };

    template <typename T>
    class NoFilter {
    public:
        
        NoFilter(float recall) {}

        void reset() {}

        void setup(const VectorStorage<T>& data, const VectorStorage<T>& queries, uint64_t seed) {
        }

        inline bool passes(size_t query_index, size_t dataset_index) const {
            return true;
        }

        inline void update(float dist) {
        }
    };
}
