#pragma once

// This is kept just for backwards compatibility
#define ALGO_VERSION 4

#define VERSION_BRUTE_FORCE                4
#define VERSION_FILTER                     1

#define VERSION_DOT_PRODUCT_i16_avx2       1
#define VERSION_DOT_PRODUCT_i16_avx512     1
#define VERSION_DOT_PRODUCT_i16_simple     1
#define VERSION_DOT_PRODUCT_float_avx2     1
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

    if (storage.compare("float_aligned")) {
        ver.storage = VERSION_STORAGE_FLOAT_ALIGNED;
    } else if (storage.compare("i16_aligned")) {
        ver.storage = VERSION_STORAGE_I16_ALIGNED;
    }

    if (method.compare("simple")) {
        if (storage.compare("float_aligned")) {
            ver.distance = VERSION_DOT_PRODUCT_float_simple;
        } else if (storage.compare("i16_aligned")) {
            ver.distance = VERSION_DOT_PRODUCT_i16_simple;
        }
    } else if (method.compare("avx512")) {
        if (storage.compare("float_aligned")) {
            ver.distance = VERSION_DOT_PRODUCT_float_avx512;
        } else if (storage.compare("i16_aligned")) {
            ver.distance = VERSION_DOT_PRODUCT_i16_avx512;
        }
    } else if (method.compare("avx2")) {
        if (storage.compare("float_aligned")) {
            ver.distance = VERSION_DOT_PRODUCT_float_avx2;
        } else if (storage.compare("i16_aligned")) {
            ver.distance = VERSION_DOT_PRODUCT_i16_avx2;
        }
    }

    return ver;
}
