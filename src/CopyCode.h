#ifndef COPYCODE_H
#define COPYCODE_H


#include <cstdint>
#include <cstddef>


typedef void (*COPYFunction)(uint8_t *pDst, ptrdiff_t nDstPitch,
                             const uint8_t *pSrc, ptrdiff_t nSrcPitch);


COPYFunction selectCopyFunction(unsigned width, unsigned height, unsigned bits);


#endif // COPYCODE_H