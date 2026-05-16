#if defined(MVTOOLS_X86)

#include <cstdint>
#include <immintrin.h>
#include <VSHelper4.h>

#define zeroes _mm256_setzero_si256()


void VerticalBilinear_avx2(uint8_t * VS_RESTRICT pDst, const uint8_t *VS_RESTRICT pSrc, intptr_t nPitch,
                           intptr_t nWidth, intptr_t nHeight, intptr_t bitsPerSample) {
    (void)bitsPerSample;

    for (int y = 0; y < nHeight - 1; y++) {
        for (int x = 0; x < nWidth; x += 32) {
            __m256i m0 = _mm256_loadu_si256((const __m256i *)&pSrc[x]);
            __m256i m1 = _mm256_loadu_si256((const __m256i *)&pSrc[x + nPitch]);

            m0 = _mm256_avg_epu8(m0, m1);
            _mm256_storeu_si256((__m256i *)&pDst[x], m0);
        }

        pSrc += nPitch;
        pDst += nPitch;
    }

    for (int x = 0; x < nWidth; x++)
        pDst[x] = pSrc[x];
}


void VerticalWiener_avx2(uint8_t * VS_RESTRICT pDst, const uint8_t *VS_RESTRICT pSrc, intptr_t nPitch,
                         intptr_t nWidth, intptr_t nHeight, intptr_t bitsPerSample) {
    (void)bitsPerSample;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < nWidth; x += 32) {
            __m256i m0 = _mm256_loadu_si256((const __m256i *)&pSrc[x]);
            __m256i m1 = _mm256_loadu_si256((const __m256i *)&pSrc[x + nPitch]);

            m0 = _mm256_avg_epu8(m0, m1);
            _mm256_storeu_si256((__m256i *)&pDst[x], m0);
        }

        pSrc += nPitch;
        pDst += nPitch;
    }

    for (int y = 2; y < nHeight - 4; y++) {
        for (int x = 0; x < nWidth; x += 16) {
            __m256i m0 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)&pSrc[x - nPitch * 2]));
            __m256i m1 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)&pSrc[x - nPitch]));
            __m256i m2 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)&pSrc[x]));
            __m256i m3 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)&pSrc[x + nPitch]));
            __m256i m4 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)&pSrc[x + nPitch * 2]));
            __m256i m5 = _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)&pSrc[x + nPitch * 3]));

            m2 = _mm256_add_epi16(m2, m3);
            m2 = _mm256_slli_epi16(m2, 2);

            m1 = _mm256_add_epi16(m1, m4);

            m2 = _mm256_sub_epi16(m2, m1);
            m3 = _mm256_slli_epi16(m2, 2);
            m2 = _mm256_add_epi16(m2, m3);

            m0 = _mm256_add_epi16(m0, m5);
            m0 = _mm256_add_epi16(m0, m2);
            m0 = _mm256_add_epi16(m0, _mm256_set1_epi16(16));

            m0 = _mm256_srai_epi16(m0, 5);
            m0 = _mm256_packus_epi16(m0, m0);
            m0 = _mm256_permute4x64_epi64(m0, _MM_SHUFFLE(0, 0, 2, 0));
            _mm_storeu_si128((__m128i *)&pDst[x], _mm256_castsi256_si128(m0));
        }

        pSrc += nPitch;
        pDst += nPitch;
    }

    for (intptr_t y = nHeight - 4; y < nHeight - 1; y++) {
        for (intptr_t x = 0; x < nWidth; x += 32) {
            __m256i m0 = _mm256_loadu_si256((const __m256i *)&pSrc[x]);
            __m256i m1 = _mm256_loadu_si256((const __m256i *)&pSrc[x + nPitch]);

            m0 = _mm256_avg_epu8(m0, m1);
            _mm256_storeu_si256((__m256i *)&pDst[x], m0);
        }

        pSrc += nPitch;
        pDst += nPitch;
    }

    for (intptr_t x = 0; x < nWidth; x++)
        pDst[x] = pSrc[x];
}


#endif // MVTOOLS_X86