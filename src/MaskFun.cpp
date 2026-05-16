// Create an overlay mask with the motion vectors
// Copyright(c)2006 A.G.Balakhnin aka Fizick
// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <algorithm>
#include <VSHelper4.h>

#include "CommonFunctions.h"
#include "CPU.h"
#include "MaskFun.h"



// Note on restrict: this function appears to always be fed memory from separate malloc calls
void CheckAndPadSmallY(int16_t * VS_RESTRICT VXSmallY, int16_t * VS_RESTRICT VYSmallY, int nBlkXP, int nBlkYP, int nBlkX, int nBlkY) {
    if (nBlkXP > nBlkX) { // fill right
        for (int j = 0; j < nBlkY; j++) {
            int16_t VXright = std::min(VXSmallY[j * nBlkXP + nBlkX - 1], (int16_t)0); // not positive
            int16_t VYright = VYSmallY[j * nBlkXP + nBlkX - 1];
            // clone: multiple 2.7.30-
            for (int dx = nBlkX; dx < nBlkXP; dx++) {
                VXSmallY[j * nBlkXP + dx] = VXright;
                VYSmallY[j * nBlkXP + dx] = VYright;
            }
        }
    }
    if (nBlkYP > nBlkY) { // fill bottom
        for (int i = 0; i < nBlkXP; i++) {
            int16_t VXbottom = VXSmallY[nBlkXP * (nBlkY - 1) + i];
            int16_t VYbottom = std::min(VYSmallY[nBlkXP * (nBlkY - 1) + i], (int16_t)0);
            for (int dy = nBlkY; dy < nBlkYP; dy++) {
                VXSmallY[nBlkXP * dy + i] = VXbottom;
                VYSmallY[nBlkXP * dy + i] = VYbottom;
            }
        }
    }
}


void CheckAndPadMaskSmall(uint8_t * VS_RESTRICT MaskSmall, int nBlkXP, int nBlkYP, int nBlkX, int nBlkY) {
    if (nBlkXP > nBlkX) { // fill right
        for (int j = 0; j < nBlkY; j++) {
            uint8_t right = MaskSmall[j * nBlkXP + nBlkX - 1];
            // clone: multiple 2.7.30-
            for (int dx = nBlkX; dx < nBlkXP; dx++) {
                MaskSmall[j * nBlkXP + dx] = right;
            }
        }
    }
    if (nBlkYP > nBlkY) { // fill bottom
        for (int i = 0; i < nBlkXP; i++) {
            uint8_t bottom = MaskSmall[nBlkXP * (nBlkY - 1) + i];
            // clone: multiple 2.7.30-
            for (int dy = nBlkY; dy < nBlkYP; dy++) {
                MaskSmall[nBlkXP * dy + i] = bottom;
            }
        }
    }
}


static inline void ByteOccMask(uint8_t * VS_RESTRICT occMask, int occlusion, double occnorm, double fGamma) {
    if (fGamma == 1.0)
        *occMask = std::max<uint8_t>(*occMask, std::min<int>(255 * occlusion * occnorm, 255));
    else
        *occMask = std::max<uint8_t>(*occMask, std::min<int>(255 * pow(occlusion * occnorm, fGamma), 255));
}

void MakeVectorOcclusionMaskTime(const FakeGroupOfPlanes *fgop, int isBackward, int nBlkX, int nBlkY, double dMaskNormDivider, double fGamma, int nPel, uint8_t * VS_RESTRICT occMask, ptrdiff_t occMaskPitch, int time256, int nBlkStepX, int nBlkStepY) { // analyse vectors field to detect occlusion
    memset(occMask, 0, occMaskPitch * nBlkY);
    int time4096X = time256 * 16 / (nBlkStepX * nPel);
    int time4096Y = time256 * 16 / (nBlkStepY * nPel);
    double occnormX = 80.0 / (dMaskNormDivider * nBlkStepX * nPel);
    double occnormY = 80.0 / (dMaskNormDivider * nBlkStepY * nPel);

    for (int by = 0; by < nBlkY; by++) {
        for (int bx = 0; bx < nBlkX; bx++) {
            int i = bx + by * nBlkX; // current block
            const FakeBlockData *block = fgopGetBlock(fgop, 0, i);
            int vx = block->vector.x;
            int vy = block->vector.y;
            if (bx < nBlkX - 1) { // right neighbor
                int i1 = i + 1;
                const FakeBlockData *block1 = fgopGetBlock(fgop, 0, i1);
                int vx1 = block1->vector.x;
                if (vx1 < vx) {
                    int occlusion = vx - vx1;
                    int minb = isBackward ? std::max(0, bx + 1 - occlusion * time4096X / 4096) : bx;
                    int maxb = isBackward ? bx + 1 : std::min(bx + 1 - occlusion * time4096X / 4096, nBlkX - 1);
                    for (int bxi = minb; bxi <= maxb; bxi++)
                        ByteOccMask(&occMask[bxi + by * occMaskPitch], occlusion, occnormX, fGamma);
                }
            }
            if (by < nBlkY - 1) { // bottom neighbor
                int i1 = i + nBlkX;
                const FakeBlockData *block1 = fgopGetBlock(fgop, 0, i1);
                int vy1 = block1->vector.y;
                if (vy1 < vy) {
                    int occlusion = vy - vy1;
                    int minb = isBackward ? std::max(0, by + 1 - occlusion * time4096Y / 4096) : by;
                    int maxb = isBackward ? by + 1 : std::min(by + 1 - occlusion * time4096Y / 4096, nBlkY - 1);
                    for (int byi = minb; byi <= maxb; byi++)
                        ByteOccMask(&occMask[bx + byi * occMaskPitch], occlusion, occnormY, fGamma);
                }
            }
        }
    }
}


static unsigned char ByteNorm(int64_t sad, double dSADNormFactor, double fGamma) {
    //        double dSADNormFactor = 4 / (dMaskNormFactor*blkSizeX*blkSizeY);
    double l = 255 * pow(sad * dSADNormFactor, fGamma); // Fizick - now linear for gm=1
    return (unsigned char)((l > 255) ? 255 : l);
}


void MakeSADMaskTime(const FakeGroupOfPlanes *fgop, int nBlkX, int nBlkY, double dSADNormFactor, double fGamma, int nPel, uint8_t * VS_RESTRICT Mask, ptrdiff_t MaskPitch, int time256, int nBlkStepX, int nBlkStepY, int bitsPerSample) {
    // Make approximate SAD mask at intermediate time
    //    double dSADNormFactor = 4 / (dMaskNormDivider*nBlkSizeX*nBlkSizeY);
    memset(Mask, 0, nBlkY * MaskPitch);
    int time4096X = (256 - time256) * 16 / (nBlkStepX * nPel); // blkstep here is really blksize-overlap
    int time4096Y = (256 - time256) * 16 / (nBlkStepY * nPel);

    for (int by = 0; by < nBlkY; by++) {
        for (int bx = 0; bx < nBlkX; bx++) {
            int i = bx + by * nBlkX; // current block
            const FakeBlockData *block = fgopGetBlock(fgop, 0, i);
            int vx = block->vector.x;
            int vy = block->vector.y;
            int bxi = bx - vx * time4096X / 4096; // limits?
            int byi = by - vy * time4096Y / 4096;
            if (bxi < 0 || bxi >= nBlkX || byi < 0 || byi >= nBlkY) {
                bxi = bx;
                byi = by;
            }
            int i1 = bxi + byi * nBlkX;
            int64_t sad = fgopGetBlock(fgop, 0, i1)->vector.sad >> (bitsPerSample - 8);
            Mask[bx + by * MaskPitch] = ByteNorm(sad, dSADNormFactor, fGamma);
        }
    }
}

// Note about restrict: it appears that this function is always called with memory allocated from different malloc calls
void MakeVectorSmallMasks(const FakeGroupOfPlanes *fgop, int nBlkX, int nBlkY, int16_t * VS_RESTRICT VXSmallY, int pitchVXSmallY, int16_t * VS_RESTRICT VYSmallY, int pitchVYSmallY) {
    // make  vector vx and vy small masks
    for (int by = 0; by < nBlkY; by++) {
        for (int bx = 0; bx < nBlkX; bx++) {
            int i = bx + by * nBlkX;
            const FakeBlockData *block = fgopGetBlock(fgop, 0, i);
            int vx = block->vector.x;
            int vy = block->vector.y;
            VXSmallY[bx + by * pitchVXSmallY] = vx; // luma
            VYSmallY[bx + by * pitchVYSmallY] = vy; // luma
        }
    }
}

void VectorSmallMaskYToHalfUV(int16_t * VS_RESTRICT VSmallY, int nBlkX, int nBlkY, int16_t *VS_RESTRICT VSmallUV, int ratioUV) {
    if (ratioUV == 2) {
        // YV12 colorformat
        for (int by = 0; by < nBlkY; by++) {
            for (int bx = 0; bx < nBlkX; bx++) {
                VSmallUV[bx] = VSmallY[bx] >> 1; // chroma
            }
            VSmallY += nBlkX;
            VSmallUV += nBlkX;
        }
    } else { // ratioUV==1
        // Height YUY2 colorformat
        for (int by = 0; by < nBlkY; by++) {
            for (int bx = 0; bx < nBlkX; bx++) {
                VSmallUV[bx] = VSmallY[bx]; // chroma
            }
            VSmallY += nBlkX;
            VSmallUV += nBlkX;
        }
    }
}


// copy refined planes to big one plane
template <typename PixelType>
static void RealMerge4PlanesToBig(uint8_t *pel2Plane_u8, ptrdiff_t pel2Pitch, const uint8_t *pPlane0_u8, const uint8_t *pPlane1_u8,
                           const uint8_t *pPlane2_u8, const uint8_t *pPlane3_u8, int width, int height, ptrdiff_t pitch) {
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            PixelType *pel2Plane = (PixelType *)pel2Plane_u8;
            const PixelType *pPlane0 = (const PixelType *)pPlane0_u8;
            const PixelType *pPlane1 = (const PixelType *)pPlane1_u8;

            pel2Plane[w << 1] = pPlane0[w];
            pel2Plane[(w << 1) + 1] = pPlane1[w];
        }
        pel2Plane_u8 += pel2Pitch;
        for (int w = 0; w < width; w++) {
            PixelType *pel2Plane = (PixelType *)pel2Plane_u8;
            const PixelType *pPlane2 = (const PixelType *)pPlane2_u8;
            const PixelType *pPlane3 = (const PixelType *)pPlane3_u8;

            pel2Plane[w << 1] = pPlane2[w];
            pel2Plane[(w << 1) + 1] = pPlane3[w];
        }
        pel2Plane_u8 += pel2Pitch;
        pPlane0_u8 += pitch;
        pPlane1_u8 += pitch;
        pPlane2_u8 += pitch;
        pPlane3_u8 += pitch;
    }
}


void Merge4PlanesToBig(uint8_t *pel2Plane, ptrdiff_t pel2Pitch, const uint8_t *pPlane0, const uint8_t *pPlane1, const uint8_t *pPlane2, const uint8_t *pPlane3, int width, int height, ptrdiff_t pitch, int bitsPerSample) {
    if (bitsPerSample == 8)
        RealMerge4PlanesToBig<uint8_t>(pel2Plane, pel2Pitch, pPlane0, pPlane1, pPlane2, pPlane3, width, height, pitch);
    else
        RealMerge4PlanesToBig<uint16_t>(pel2Plane, pel2Pitch, pPlane0, pPlane1, pPlane2, pPlane3, width, height, pitch);
}


// copy refined planes to big one plane
template <typename PixelType>
static void RealMerge16PlanesToBig(uint8_t * VS_RESTRICT pel4Plane_u8, ptrdiff_t pel4Pitch,
                            const uint8_t * VS_RESTRICT pPlane0_u8, const uint8_t * VS_RESTRICT pPlane1_u8, const uint8_t * VS_RESTRICT pPlane2_u8, const uint8_t * VS_RESTRICT pPlane3_u8,
                            const uint8_t * VS_RESTRICT pPlane4_u8, const uint8_t * VS_RESTRICT pPlane5_u8, const uint8_t * VS_RESTRICT pPlane6_u8, const uint8_t * VS_RESTRICT pPlane7_u8,
                            const uint8_t * VS_RESTRICT pPlane8_u8, const uint8_t * VS_RESTRICT pPlane9_u8, const uint8_t * VS_RESTRICT pPlane10_u8, const uint8_t * VS_RESTRICT pPlane11_u8,
                            const uint8_t * VS_RESTRICT pPlane12_u8, const uint8_t * VS_RESTRICT pPlane13_u8, const uint8_t * VS_RESTRICT pPlane14_u8, const uint8_t * VS_RESTRICT pPlane15_u8,
                            int width, int height, ptrdiff_t pitch) {
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            PixelType *pel4Plane = (PixelType *)pel4Plane_u8;
            const PixelType *pPlane0 = (const PixelType *)pPlane0_u8;
            const PixelType *pPlane1 = (const PixelType *)pPlane1_u8;
            const PixelType *pPlane2 = (const PixelType *)pPlane2_u8;
            const PixelType *pPlane3 = (const PixelType *)pPlane3_u8;

            pel4Plane[w << 2] = pPlane0[w];
            pel4Plane[(w << 2) + 1] = pPlane1[w];
            pel4Plane[(w << 2) + 2] = pPlane2[w];
            pel4Plane[(w << 2) + 3] = pPlane3[w];
        }
        pel4Plane_u8 += pel4Pitch;
        for (int w = 0; w < width; w++) {
            PixelType *pel4Plane = (PixelType *)pel4Plane_u8;
            const PixelType *pPlane4 = (const PixelType *)pPlane4_u8;
            const PixelType *pPlane5 = (const PixelType *)pPlane5_u8;
            const PixelType *pPlane6 = (const PixelType *)pPlane6_u8;
            const PixelType *pPlane7 = (const PixelType *)pPlane7_u8;

            pel4Plane[w << 2] = pPlane4[w];
            pel4Plane[(w << 2) + 1] = pPlane5[w];
            pel4Plane[(w << 2) + 2] = pPlane6[w];
            pel4Plane[(w << 2) + 3] = pPlane7[w];
        }
        pel4Plane_u8 += pel4Pitch;
        for (int w = 0; w < width; w++) {
            PixelType *pel4Plane = (PixelType *)pel4Plane_u8;
            const PixelType *pPlane8 = (const PixelType *)pPlane8_u8;
            const PixelType *pPlane9 = (const PixelType *)pPlane9_u8;
            const PixelType *pPlane10 = (const PixelType *)pPlane10_u8;
            const PixelType *pPlane11 = (const PixelType *)pPlane11_u8;

            pel4Plane[w << 2] = pPlane8[w];
            pel4Plane[(w << 2) + 1] = pPlane9[w];
            pel4Plane[(w << 2) + 2] = pPlane10[w];
            pel4Plane[(w << 2) + 3] = pPlane11[w];
        }
        pel4Plane_u8 += pel4Pitch;
        for (int w = 0; w < width; w++) {
            PixelType *pel4Plane = (PixelType *)pel4Plane_u8;
            const PixelType *pPlane12 = (const PixelType *)pPlane12_u8;
            const PixelType *pPlane13 = (const PixelType *)pPlane13_u8;
            const PixelType *pPlane14 = (const PixelType *)pPlane14_u8;
            const PixelType *pPlane15 = (const PixelType *)pPlane15_u8;

            pel4Plane[w << 2] = pPlane12[w];
            pel4Plane[(w << 2) + 1] = pPlane13[w];
            pel4Plane[(w << 2) + 2] = pPlane14[w];
            pel4Plane[(w << 2) + 3] = pPlane15[w];
        }
        pel4Plane_u8 += pel4Pitch;
        pPlane0_u8 += pitch;
        pPlane1_u8 += pitch;
        pPlane2_u8 += pitch;
        pPlane3_u8 += pitch;
        pPlane4_u8 += pitch;
        pPlane5_u8 += pitch;
        pPlane6_u8 += pitch;
        pPlane7_u8 += pitch;
        pPlane8_u8 += pitch;
        pPlane9_u8 += pitch;
        pPlane10_u8 += pitch;
        pPlane11_u8 += pitch;
        pPlane12_u8 += pitch;
        pPlane13_u8 += pitch;
        pPlane14_u8 += pitch;
        pPlane15_u8 += pitch;
    }
}


void Merge16PlanesToBig(uint8_t *pel4Plane, ptrdiff_t pel4Pitch,
                        const uint8_t *pPlane0, const uint8_t *pPlane1, const uint8_t *pPlane2, const uint8_t *pPlane3,
                        const uint8_t *pPlane4, const uint8_t *pPlane5, const uint8_t *pPlane6, const uint8_t *pPlane7,
                        const uint8_t *pPlane8, const uint8_t *pPlane9, const uint8_t *pPlane10, const uint8_t *pPlane11,
                        const uint8_t *pPlane12, const uint8_t *pPlane13, const uint8_t *pPlane14, const uint8_t *pPlane15,
                        int width, int height, ptrdiff_t pitch, int bitsPerSample) {
    if (bitsPerSample == 8)
        RealMerge16PlanesToBig<uint8_t>(pel4Plane, pel4Pitch, pPlane0, pPlane1, pPlane2, pPlane3, pPlane4, pPlane5, pPlane6, pPlane7, pPlane8, pPlane9, pPlane10, pPlane11, pPlane12, pPlane13, pPlane14, pPlane15, width, height, pitch);
    else
        RealMerge16PlanesToBig<uint16_t>(pel4Plane, pel4Pitch, pPlane0, pPlane1, pPlane2, pPlane3, pPlane4, pPlane5, pPlane6, pPlane7, pPlane8, pPlane9, pPlane10, pPlane11, pPlane12, pPlane13, pPlane14, pPlane15, width, height, pitch);
}


// time-weihted blend src with ref frames (used for interpolation for poor motion estimation)
template <typename PixelType>
static void RealBlend(uint8_t * VS_RESTRICT pdst, const uint8_t * VS_RESTRICT psrc, const uint8_t * VS_RESTRICT pref, int height, int width, ptrdiff_t dst_pitch, ptrdiff_t src_pitch, ptrdiff_t ref_pitch, int time256) {
    int h, w;
    for (h = 0; h < height; h++) {
        for (w = 0; w < width; w++) {
            const PixelType *psrc_ = (const PixelType *)psrc;
            const PixelType *pref_ = (const PixelType *)pref;
            PixelType *pdst_ = (PixelType *)pdst;

            pdst_[w] = (psrc_[w] * (256 - time256) + pref_[w] * time256) >> 8;
        }
        pdst += dst_pitch;
        psrc += src_pitch;
        pref += ref_pitch;
    }
}


void Blend(uint8_t *pdst, const uint8_t *psrc, const uint8_t *pref, int height, int width, ptrdiff_t dst_pitch, ptrdiff_t src_pitch, ptrdiff_t ref_pitch, int time256, int bitsPerSample) {
    if (bitsPerSample == 8)
        RealBlend<uint8_t>(pdst, psrc, pref, height, width, dst_pitch, src_pitch, ref_pitch, time256);
    else
        RealBlend<uint16_t>(pdst, psrc, pref, height, width, dst_pitch, src_pitch, ref_pitch, time256);
}


template <typename PixelType>
static void FlowInter(
        uint8_t * VS_RESTRICT pdst8, ptrdiff_t dst_pitch,
        const uint8_t *prefB8, const uint8_t *prefF8, ptrdiff_t ref_pitch,
        const int16_t *VXFullB, const int16_t *VXFullF,
        const int16_t *VYFullB, const int16_t *VYFullF,
        const uint8_t *MaskB, const uint8_t *MaskF, ptrdiff_t VPitch,
        int width, int height,
        int time256, int nPel) {

    const PixelType *prefB = (const PixelType *)prefB8;
    const PixelType *prefF = (const PixelType *)prefF8;
    PixelType *pdst = (PixelType *)pdst8;

    ref_pitch /= sizeof(PixelType);
    dst_pitch /= sizeof(PixelType);

    int nPelLog = ilog2(nPel);

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int vxF = (VXFullF[w] * time256) >> 8;
            int vyF = (VYFullF[w] * time256) >> 8;
            int64_t dstF = prefF[vyF * ref_pitch + vxF + (w << nPelLog)];
            int dstF0 = prefF[(w << nPelLog)]; /* zero */
            int vxB = (VXFullB[w] * (256 - time256)) >> 8;
            int vyB = (VYFullB[w] * (256 - time256)) >> 8;
            int64_t dstB = prefB[vyB * ref_pitch + vxB + (w << nPelLog)];
            int dstB0 = prefB[(w << nPelLog)]; /* zero */
            pdst[w] = (PixelType)((((dstF * (256 - MaskF[w]) + ((MaskF[w] * (dstB * (256 - MaskB[w]) + MaskB[w] * dstF0) + 256) >> 8) + 256) >> 8) * (256 - time256) +
                ((dstB * (256 - MaskB[w]) + ((MaskB[w] * (dstF * (256 - MaskF[w]) + MaskF[w] * dstB0) + 256) >> 8) + 256) >> 8) * time256) >>
                8) - 1;
        }
        pdst += dst_pitch;
        prefB += ref_pitch << nPelLog;
        prefF += ref_pitch << nPelLog;
        VXFullB += VPitch;
        VYFullB += VPitch;
        VXFullF += VPitch;
        VYFullF += VPitch;
        MaskB += VPitch;
        MaskF += VPitch;
    }
}


template <typename PixelType>
static void FlowInterExtra(
        uint8_t * VS_RESTRICT pdst8, ptrdiff_t dst_pitch,
        const uint8_t *prefB8, const uint8_t *prefF8, ptrdiff_t ref_pitch,
        const int16_t *VXFullB, const int16_t *VXFullF,
        const int16_t *VYFullB, const int16_t *VYFullF,
        const uint8_t *MaskB, const uint8_t *MaskF, ptrdiff_t VPitch,
        int width, int height,
        int time256, int nPel,
        const int16_t *VXFullBB, const int16_t *VXFullFF,
        const int16_t *VYFullBB, const int16_t *VYFullFF) {

    const PixelType *prefB = (const PixelType *)prefB8;
    const PixelType *prefF = (const PixelType *)prefF8;
    PixelType *pdst = (PixelType *)pdst8;

    ref_pitch /= sizeof(PixelType);
    dst_pitch /= sizeof(PixelType);

    int nPelLog = ilog2(nPel);

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            int vxF = (VXFullF[w] * time256) >> 8;
            int vyF = (VYFullF[w] * time256) >> 8;
            int adrF = vyF * ref_pitch + vxF + (w << nPelLog);
            int dstF = prefF[adrF];

            int vxFF = (VXFullFF[w] * time256) >> 8;
            int vyFF = (VYFullFF[w] * time256) >> 8;
            int dstFF = prefF[vyFF * ref_pitch + vxFF + (w << nPelLog)];

            int vxB = (VXFullB[w] * (256 - time256)) >> 8;
            int vyB = (VYFullB[w] * (256 - time256)) >> 8;
            int dstB = prefB[vyB * ref_pitch + vxB + (w << nPelLog)];

            int vxBB = (VXFullBB[w] * (256 - time256)) >> 8;
            int vyBB = (VYFullBB[w] * (256 - time256)) >> 8;
            int dstBB = prefB[vyBB * ref_pitch + vxBB + (w << nPelLog)];

            /* use median, firsly get min max of compensations */
            int minfb = std::min(dstB, dstF);
            int maxfb = std::max(dstB, dstF);

            int medianBB = std::max(minfb, std::min(dstBB, maxfb));
            int medianFF = std::max(minfb, std::min(dstFF, maxfb));

            pdst[w] = ((((medianBB * MaskF[w] + dstF * (256 - MaskF[w]) + 256) >> 8) * (256 - time256) +
                ((medianFF * MaskB[w] + dstB * (256 - MaskB[w]) + 256) >> 8) * time256) >>
                8) - 1;
        }
        pdst += dst_pitch;
        prefB += ref_pitch << nPelLog;
        prefF += ref_pitch << nPelLog;
        VXFullB += VPitch;
        VYFullB += VPitch;
        VXFullF += VPitch;
        VYFullF += VPitch;
        MaskB += VPitch;
        MaskF += VPitch;
        VXFullBB += VPitch;
        VYFullBB += VPitch;
        VXFullFF += VPitch;
        VYFullFF += VPitch;
    }
}


template <typename PixelType>
static void FlowInterSimple(
        uint8_t * VS_RESTRICT pdst8, ptrdiff_t dst_pitch,
        const uint8_t *prefB8, const uint8_t *prefF8, ptrdiff_t ref_pitch,
        const int16_t *VXFullB, const int16_t *VXFullF,
        const int16_t *VYFullB, const int16_t *VYFullF,
        const uint8_t *MaskB, const uint8_t *MaskF, ptrdiff_t VPitch,
        int width, int height,
        int time256, int nPel) {

    const PixelType *prefB = (const PixelType *)prefB8;
    const PixelType *prefF = (const PixelType *)prefF8;
    PixelType *pdst = (PixelType *)pdst8;

    ref_pitch /= sizeof(PixelType);
    dst_pitch /= sizeof(PixelType);

    int nPelLog = ilog2(nPel);

    if (time256 == 128) { /* special case double fps - fastest */
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                int vxF = VXFullF[w] >> 1;
                int vyF = VYFullF[w] >> 1;
                int dstF = prefF[vyF * ref_pitch + vxF + (w << nPelLog)];
                int vxB = VXFullB[w] >> 1;
                int vyB = VYFullB[w] >> 1;
                int dstB = prefB[vyB * ref_pitch + vxB + (w << nPelLog)];
                pdst[w] = (((dstF + dstB) << 8) + (dstB - dstF) * (MaskF[w] - MaskB[w])) >> 9;
            }
            pdst += dst_pitch;
            prefB += ref_pitch << nPelLog;
            prefF += ref_pitch << nPelLog;
            VXFullB += VPitch;
            VYFullB += VPitch;
            VXFullF += VPitch;
            VYFullF += VPitch;
            MaskB += VPitch;
            MaskF += VPitch;
        }
    } else { /* general case */
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                int vxF = (VXFullF[w] * time256) >> 8;
                int vyF = (VYFullF[w] * time256) >> 8;
                int dstF = prefF[vyF * ref_pitch + vxF + (w << nPelLog)];
                int vxB = (VXFullB[w] * (256 - time256)) >> 8;
                int vyB = (VYFullB[w] * (256 - time256)) >> 8;
                int dstB = prefB[vyB * ref_pitch + vxB + (w << nPelLog)];
                pdst[w] = (((dstF * (255 - MaskF[w]) + dstB * MaskF[w] + 255) >> 8) * (256 - time256) +
                           ((dstB * (255 - MaskB[w]) + dstF * MaskB[w] + 255) >> 8) * time256) >>
                          8;
            }
            pdst += dst_pitch;
            prefB += ref_pitch << nPelLog;
            prefF += ref_pitch << nPelLog;
            VXFullB += VPitch;
            VYFullB += VPitch;
            VXFullF += VPitch;
            VYFullF += VPitch;
            MaskB += VPitch;
            MaskF += VPitch;
        }
    }
}


void selectFlowInterFunctions(FlowInterSimpleFunction *simple, FlowInterFunction *regular, FlowInterExtraFunction *extra, int bitsPerSample, int opt) {
    if (bitsPerSample == 8) {
        *simple = FlowInterSimple<uint8_t>;
        *regular = FlowInter<uint8_t>;
        *extra = FlowInterExtra<uint8_t>;
    } else {
        *simple = FlowInterSimple<uint16_t>;
        *regular = FlowInter<uint16_t>;
        *extra = FlowInterExtra<uint16_t>;
    }
}
