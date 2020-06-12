#pragma once

#if defined(__AVX512_F__) || defined(__AVX2__) || defined(__AVX__)
    #include <immintrin.h>
#endif

namespace toy_project {
    #ifdef __AVX512_F__
        static int16_t dot_product_i16_avx512(const int16_t* lhs, const int16_t* rhs, unsigned int dimensions) {
            // Number of i16 values that fit into a 512 bit vector.
            const static unsigned int VALUES_PER_VEC = 32;

            // specialized function for multiplication of the fixed point format
            __m512i res = _mm512_mulhrs_epi16(
                _mm512_load_si512((__m512i*)&lhs[0]),
                _mm512_load_si512((__m512i*)&rhs[0]));
            for (
                unsigned int i=VALUES_PER_VEC;
                i < dimensions;
                i += VALUES_PER_VEC
            ) {
                __m512i tmp = _mm512_mulhrs_epi16(
                    _mm512_load_si512((__m512i*)&lhs[i]),
                    _mm512_load_si512((__m512i*)&rhs[i]));
                res = _mm512_add_epi16(res, tmp);
            }
            alignas(32) int16_t stored[VALUES_PER_VEC];
            _mm512_store_si512((__m512i*)stored, res);
            int16_t ret = 0;
            for (unsigned i=0; i<VALUES_PER_VEC; i++) { ret += stored[i]; }
            return ret;
        }
    #elif __AVX2__
        static int16_t dot_product_i16_avx512(const int16_t* lhs, const int16_t* rhs, unsigned int dimensions) {
            throw std::runtime_error("AVX-512 not available");
        }
        static int16_t dot_product_i16_avx2(const int16_t* lhs, const int16_t* rhs, unsigned int dimensions) {
            // Number of i16 values that fit into a 256 bit vector.
            const static unsigned int VALUES_PER_VEC = 16;

            // specialized function for multiplication of the fixed point format
            __m256i res = _mm256_mulhrs_epi16(
                _mm256_load_si256((__m256i*)&lhs[0]),
                _mm256_load_si256((__m256i*)&rhs[0]));
            for (
                unsigned int i=VALUES_PER_VEC;
                i < dimensions;
                i += VALUES_PER_VEC
            ) {
                __m256i tmp = _mm256_mulhrs_epi16(
                    _mm256_load_si256((__m256i*)&lhs[i]),
                    _mm256_load_si256((__m256i*)&rhs[i]));
                res = _mm256_add_epi16(res, tmp);
            }
            alignas(32) int16_t stored[VALUES_PER_VEC];
            _mm256_store_si256((__m256i*)stored, res);
            int16_t ret = 0;
            for (unsigned i=0; i<VALUES_PER_VEC; i++) { ret += stored[i]; }
            return ret;
        }
    #else
        static int16_t dot_product_i16_avx512(const int16_t* lhs, const int16_t* rhs, unsigned int dimensions) {
            throw std::runtime_error("AVX-512 not available");
        }
        static int16_t dot_product_i16_avx2(const int16_t* lhs, const int16_t* rhs, unsigned int dimensions) {
            throw std::runtime_error("AVX2 not available");
        }
    #endif

    static int16_t dot_product_i16_simple(const int16_t* lhs, const int16_t* rhs, unsigned int dimensions) {
        int16_t res = 0;
        for (unsigned int i=0; i < dimensions; i++) {
            int32_t precise = static_cast<int32_t>(lhs[i])*static_cast<int32_t>(rhs[i]);
            res += static_cast<int16_t>(((precise >> 14)+1) >> 1);
        }
        return res;
    }


    static float dot_product_simple(const float* lhs, const float* rhs, unsigned int dimensions) {
        float res = 0.0f;
        for (unsigned int i=0; i < dimensions; i++) {
            res += lhs[i] * rhs[i];
        }
        return res;
    }

    #ifdef __AVX_512_F__
        // Compute the l2 distance between two floating point vectors without taking the
        // final root.
        static float l2_distance_float_avx512(const float* lhs, const float* rhs, unsigned int dimensions) {
            // Number of float values that fit into a 512 bit vector.
            const static unsigned int VALUES_PER_VEC = 16;

            __m512 res = _mm512_sub_ps(
                _mm512_load_ps(&lhs[0]),
                _mm512_load_ps(&rhs[0]));
            res = _mm512_mul_ps(res, res);

            for (
                unsigned int i=VALUES_PER_VEC;
                i < dimensions;
                i += VALUES_PER_VEC
            ) {
                __m512 tmp = _mm512_sub_ps(
                    _mm512_load_ps(&lhs[i]),
                    _mm512_load_ps(&rhs[i]));
                tmp = _mm512_mul_ps(tmp, tmp);
                res = _mm512_add_ps(res, tmp);
            }
            alignas(32) float stored[VALUES_PER_VEC];
            _mm512_store_ps(stored, res);
            float ret = 0;
            for (unsigned i=0; i < VALUES_PER_VEC; i++) {
                ret += stored[i];
            }
            return ret;
        }

    #elif __AVX__
        static float l2_distance_float_avx512(const float* lhs, const float* rhs, unsigned int dimensions) {
            throw std::runtime_error("AVX 512 not available");
        }

        // Compute the l2 distance between two floating point vectors without taking the
        // final root.
        static float l2_distance_float_avx2(const float* lhs, const float* rhs, unsigned int dimensions) {
            // Number of float values that fit into a 256 bit vector.
            const static unsigned int VALUES_PER_VEC = 8;

            __m256 res = _mm256_sub_ps(
                _mm256_load_ps(&lhs[0]),
                _mm256_load_ps(&rhs[0]));
            res = _mm256_mul_ps(res, res);

            for (
                unsigned int i=VALUES_PER_VEC;
                i < dimensions;
                i += VALUES_PER_VEC
            ) {
                __m256 tmp = _mm256_sub_ps(
                    _mm256_load_ps(&lhs[i]),
                    _mm256_load_ps(&rhs[i]));
                tmp = _mm256_mul_ps(tmp, tmp);
                res = _mm256_add_ps(res, tmp);
            }
            alignas(32) float stored[VALUES_PER_VEC];
            _mm256_store_ps(stored, res);
            float ret = 0;
            for (unsigned i=0; i < VALUES_PER_VEC; i++) {
                ret += stored[i];
            }
            return ret;
        }
    #else
        static float l2_distance_float_avx512(const float* lhs, const float* rhs, unsigned int dimensions) {
            throw std::runtime_error("AVX 512 not available");
        }

        // Compute the l2 distance between two floating point vectors without taking the
        // final root.
        static float l2_distance_float_avx2(const float* lhs, const float* rhs, unsigned int dimensions) {
            throw std::runtime_error("AVX 512 not available");
        }
    #endif

    static float l2_distance_float_simple(const float* lhs, const float* rhs, unsigned int dimensions) {
        float res = 0.0;
        for (unsigned int i=0; i < dimensions; i++) {
            float diff = lhs[i]-rhs[i];
            res += diff*diff;
        }
        return res;
    }

    struct Angular {
        using t = int16_t;
        static constexpr t (*simple_distance)(const t*, const t*, unsigned int) = &dot_product_i16_simple;
        static constexpr t (*avx2_distance)(const t*, const t*, unsigned int) = &dot_product_i16_avx2;
        static constexpr t (*avx512_distance)(const t*, const t*, unsigned int) = &dot_product_i16_avx512;
    };

    struct L2 {
        using t = float;
        static constexpr t (*simple_distance)(const t*, const t*, unsigned int) = &l2_distance_float_simple;
        static constexpr t (*avx2_distance)(const t*, const t*, unsigned int) = &l2_distance_float_avx2;
        static constexpr t (*avx512_distance)(const t*, const t*, unsigned int) = &l2_distance_float_avx512;
    };


}
