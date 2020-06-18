#pragma once

#include <iostream>

// This is kept just for backwards compatibility
#define ALGO_VERSION 4

// The macros below define the current version of each component
// of the software. Version numbers should only be *increased*
// by one.
#define VERSION_BRUTE_FORCE                1
#define VERSION_FILTER                     1

#define VERSION_DOT_PRODUCT_i16_avx2       1
#define VERSION_DOT_PRODUCT_i16_avx512     1
#define VERSION_DOT_PRODUCT_i16_simple     1
#define VERSION_DOT_PRODUCT_float_avx2     2
#define VERSION_DOT_PRODUCT_float_avx512   1
#define VERSION_DOT_PRODUCT_float_simple   1

#define VERSION_STORAGE_FLOAT_ALIGNED      1
#define VERSION_STORAGE_I16_ALIGNED        1

struct Version {
    std::string components;
    uint32_t algo_version;
    uint32_t distance;
    uint32_t storage;
    uint32_t filter;
    uint32_t brute_force;
};

Version get_version(bool filter, std::string method, std::string storage) {
    Version ver;
    ver.algo_version = ALGO_VERSION;
    ver.brute_force = VERSION_BRUTE_FORCE;

    std::stringstream sstr;
    if (filter) {
        ver.filter = VERSION_FILTER;
        sstr << "filter_";
    } else {
        ver.filter = 0;
        sstr << "nofilter_";
    }
    sstr << storage << "_" << method;
    ver.components = sstr.str();

    if (storage.compare("float_aligned") == 0) {
        ver.storage = VERSION_STORAGE_FLOAT_ALIGNED;
    } else if (storage.compare("i16_aligned") == 0) {
        ver.storage = VERSION_STORAGE_I16_ALIGNED;
    } else {
        throw std::runtime_error("unknown storage");
    }

    if (method.compare("simple") == 0) {
        if (storage.compare("float_aligned") == 0) {
            ver.distance = VERSION_DOT_PRODUCT_float_simple;
        } else if (storage.compare("i16_aligned") == 0) {
            ver.distance = VERSION_DOT_PRODUCT_i16_simple;
        }
    } else if (method.compare("avx512") == 0) {
        if (storage.compare("float_aligned") == 0) {
            ver.distance = VERSION_DOT_PRODUCT_float_avx512;
        } else if (storage.compare("i16_aligned") == 0) {
            ver.distance = VERSION_DOT_PRODUCT_i16_avx512;
        }
    } else if (method.compare("avx2") == 0) {
        if (storage.compare("float_aligned") == 0) {
            ver.distance = VERSION_DOT_PRODUCT_float_avx2;
        } else if (storage.compare("i16_aligned") == 0) {
            ver.distance = VERSION_DOT_PRODUCT_i16_avx2;
        }
    } else {
        throw std::runtime_error("unknown method");
    }

    return ver;
}
