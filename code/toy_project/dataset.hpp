#pragma once

#include "generic.hpp"
#include "typedefs.hpp"

#include <cstring>
#include <istream>
#include <memory>
#include <ostream>

namespace toy_project {
    // The container for all inserted vectors.
    // The data is stored according to the given format.
    template <typename T>
    class Dataset {
        // Arguments
        typename T::Args args;
        // Number of dimensions of stored vectors, which may be padded to
        // more easily map to simd instructions.
        unsigned int storage_len;
        // Number of inserted vectors.
        unsigned int inserted_vectors;
        // Maximal number of inserted vectors.
        unsigned int capacity;
        // Inserted vectors, aligned to the vector alignment.
        AlignedStorage<T> data;

    public:
        // Create an empty storage for vectors with the given number of dimensions.
        // Allocates enough space for the given number of vectors before needing to reallocate.
        Dataset(typename T::Args args, unsigned int capacity)
          : args(args),
            storage_len(pad_dimensions<T>(T::storage_dimensions(args))),
            inserted_vectors(0),
            capacity(capacity),
            data(allocate_storage<T>(capacity, storage_len))
        {
        }

        Dataset(Dataset&& other)
          : args(other.args),
            storage_len(other.storage_len),
            inserted_vectors(other.inserted_vectors),
            capacity(other.capacity),
            data(std::move(other.data))
        {
        }

        Dataset& operator=(Dataset&& rhs) {
            if (this != &rhs) {
                args = rhs.args;
                storage_len = rhs.storage_len;
                inserted_vectors = rhs.inserted_vectors;
                capacity = rhs.capacity;
                data = std::move(rhs.data);
            }
            return *this;
        }

        // Access the vector at the given position.
        typename T::Type* operator[](unsigned int idx) const {
            return &data.get()[idx*storage_len];
        }

        // Retrieve the number of inserted vectors.
        unsigned int get_size() const {
            return inserted_vectors;
        }

        // Retrieve the number of dimensions of vectors inserted into this dataset,
        // as well as the number of dimensions they are stored with.
        DatasetDescription<T> get_description() const {
            DatasetDescription<T> res;
            res.args = args;
            res.storage_len = storage_len;
            return res;
        }

        // Insert a vector.
        template <typename U>
        void insert(const U& vec) {
            T::store(
                vec,
                &data.get()[inserted_vectors*storage_len],
                get_description());
            inserted_vectors++;
        }


    };
}
