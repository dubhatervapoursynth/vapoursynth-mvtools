#ifndef LUMA_H
#define LUMA_H


#include <cstdint>


typedef unsigned int (*LUMAFunction)(const uint8_t *pSrc, ptrdiff_t nSrcPitch);


LUMAFunction selectLumaFunction(unsigned width, unsigned height, unsigned bits, int opt);


#endif // LUMA_H
