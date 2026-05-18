#ifndef OVERLAP_H
#define OVERLAP_H


#include <cstdint>
#include <algorithm>
#include <VSHelper4.h>

// top, middle, botom and left, middle, right windows
#define OW_TL 0
#define OW_TM 1
#define OW_TR 2
#define OW_ML 3
#define OW_MM 4
#define OW_MR 5
#define OW_BL 6
#define OW_BM 7
#define OW_BR 8

typedef struct OverlapWindows {
    int nx; // window sizes
    int ny;
    int ox; // overap sizes
    int oy;
    int size; // full window size= nx*ny

    int16_t *Overlap9Windows;

    float *fWin1UVx;
    float *fWin1UVxfirst;
    float *fWin1UVxlast;
    float *fWin1UVy;
    float *fWin1UVyfirst;
    float *fWin1UVylast;
} OverlapWindows;

void overInit(OverlapWindows *over, int nx, int ny, int ox, int oy);

void overDeinit(OverlapWindows *over);

int16_t *overGetWindow(const OverlapWindows *over, int i);


typedef void (*OverlapsFunction)(uint8_t *pDst, ptrdiff_t nDstPitch,
                                 const uint8_t *pSrc, ptrdiff_t nSrcPitch,
                                 const int16_t *pWin, ptrdiff_t nWinPitch);


typedef void (*ToPixelsFunction)(uint8_t *pDst, ptrdiff_t nDstPitch,
                                 const uint8_t *pSrc, ptrdiff_t nSrcPitch,
                                 int width, int height, int bitsPerSample);

template<typename PixelType2, typename PixelType>
void ToPixels(uint8_t *VS_RESTRICT pDst8, ptrdiff_t nDstPitch, const uint8_t *VS_RESTRICT pSrc8, ptrdiff_t nSrcPitch, int nWidth, int nHeight, int bitsPerSample) {
    int pixelMax = (1 << bitsPerSample) - 1;

    for (int h = 0; h < nHeight; h++) {
        for (int i = 0; i < nWidth; i++) {
            const PixelType2 *pSrc = (const PixelType2 *)pSrc8;
            PixelType *pDst = (PixelType *)pDst8;

            int a = (pSrc[i] + 16) >> 5;
            if (sizeof(PixelType) == 1)
                pDst[i] = a | ((255 - a) >> (sizeof(int) * 8 - 1));
            else
                pDst[i] = std::min(pixelMax, a);
        }
        pDst8 += nDstPitch;
        pSrc8 += nSrcPitch;
    }
}   

OverlapsFunction selectOverlapsFunction(unsigned width, unsigned height, unsigned bits, int opt);

#if defined(MVTOOLS_X86)
OverlapsFunction selectOverlapsFunctionAVX2(unsigned width, unsigned height, unsigned bits);
#endif


#endif // OVERLAP_H
