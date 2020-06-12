#pragma once

#include <string>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <valarray>
#include <vector>
#include <H5Cpp.h>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

struct DatasetConfig {
    std::string path;
    int version;
};

// I'm using std::valarray just to be able to use the convenience of std::slice.
// however, according to the internet, valarray is some sort of sad orphan
// in the standard library.
//
// Maybe a better option is to use blitz++ or to just convert everything to
// a vector of vectors
class Dataset {
    std::valarray<float> inner;
    size_t n;
    size_t dim;

    public:
    Dataset(std::valarray<float> inner, size_t n, size_t dim): inner(inner), n(n), dim(dim) {}

    // I think there is a copy going on here, which is undesirable
    std::valarray<float> get(size_t i) {
        return inner[std::slice(i * dim, dim, 1)];
    }

    int num_vectors() {
        return n;
    }

    int dimension() {
        return dim;
    }
};

DatasetConfig dataset_path(std::string name) {
    // Make the path of the python script relative to the directory
    std::stringstream sstr;
    sstr << "python3 ./datasets.py " << name;
    std::string command_output = exec(sstr.str().c_str());
    std::stringstream csstr(command_output);
    DatasetConfig config;
    std::getline(csstr, config.path, '\n');
    csstr >> config.version;
    return config;
}

Dataset load(std::string name) {

    DatasetConfig conf = dataset_path(name);
    std::stringstream sstr;
    sstr << conf.version;

    // Open the file and get the dataset
    H5::H5File file( conf.path, H5F_ACC_RDONLY );
    H5::Group group = file.openGroup(sstr.str());
    H5::DataSet dataset = group.openDataSet( "vectors" );
    H5T_class_t type_class = dataset.getTypeClass();
    // Check that we have a dataset of floats
    if (type_class != H5T_FLOAT) { throw std::runtime_error("wrong type class"); }
    H5::DataSpace dataspace = dataset.getSpace();

    // Get the number of vectors and the dimensionality
    hsize_t dims_out[2];
    dataspace.getSimpleExtentDims(dims_out, NULL);
    
    // Read the output into a row-major vector.
    // It is super cumbersome to read it into a vector 
    // of vectors. The general advice is to handle 
    // indexing manually, perhaps with the help of
    // std::slice
    std::valarray<float> output(dims_out[0] * dims_out[1]);
    dataset.read(&output[0], H5::PredType::NATIVE_FLOAT);

    return Dataset(output, dims_out[0], dims_out[1]);
}
