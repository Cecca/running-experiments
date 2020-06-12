#pragma once

#include <chrono>
#include <cstdint>
#include <random>

namespace toy_project {
    // Number of bits used in filtering sketches.
    const static unsigned int NUM_FILTER_HASHBITS = 64;
    using FilterLshDatatype = uint64_t;

    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

    // Retrieve the default random engine, seeded once by the system clock.
    std::default_random_engine& get_default_random_generator() {
        return generator;
    }

    #if defined(__GNUC__)
        #define popcountll __builtin_popcountll
    # elif defined(_MSC_VER)
        #define popcountll __popcnt64
    #else
        // Code from paper:
        // Faster Population Counts Using AVX2 Instructions
        // Wojciech Mula, Nathan Kurz and Daniel Lemire
        uint64_t popcountll(uint64_t x) {
            uint64_t c1 = 0x5555555555555555llu;
            uint64_t c2 = 0x3333333333333333llu;
            uint64_t c4 = 0x0F0F0F0F0F0F0F0Fllu;

            x -= (x >> 1) & c1;
            x = ((x >> 2) & c2) + (x & c2);
            x = (x + (x >> 4)) & c4;
            x *= 0x0101010101010101llu;
            return x >> 56;
        }
    #endif

    #if defined(__GNUC__)
        #define prefetch_addr __builtin_prefetch
    #else
        void prefetch_addr(void*) { /* noop */ }
    #endif
 }
