/*
Copyright (c) 2019 Luis Pons

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.

    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "FastQuantizer.h"

#define ASSERT assert

typedef struct {
  int wHeadIdx;     // Head point of the cluster
  int wFarthestIdx; //
  int wStartIdx;    // Starting point for the cluster in the array
  int wLenCluster;  // Number of samples in this cluster
} TCluster;

typedef struct {
  TCluster *pClusters;
  int wNumClusters;
  TDataFour *pSamples;
  TDistance *pDistancesToHead;
  int *pOriginalPositions;
} TWorkBuffers;

struct SAQuantizer {
  int wMaxSamples;
  int wMaxNumSubSamples;
  int wNDimensions;
  unsigned int uwUserWeights;
  TDistance *pTmpDistances;
  TWorkBuffers *pBuffersSrc;
  TWorkBuffers *pBuffersDst;
};

static inline int GetComp(TDataFour wSamples, int wIndex) {
  return ((wSamples >> (wIndex << 3)) & 0xff);
}

static inline void SetComp(TDataFour *pSamples, int wIndex,
                           unsigned int uwData) {
  int wShift = wIndex << 3;
  unsigned int wMask = 0xff << wShift;
  *pSamples = (*pSamples & (~wMask)) | (uwData << wShift);
}

static inline TDistance SquaredDistance(TDataFour uwA, TDataFour uwB,
                                        unsigned int uwUserWeights) {
  int wDelta = 0;
  int wDist = 0;
  wDelta = GetComp(uwA, 3) - GetComp(uwB, 3);
  wDist += wDelta * wDelta * ((uwUserWeights >> 24) & 0xff);
  wDelta = GetComp(uwA, 2) - GetComp(uwB, 2);
  wDist += wDelta * wDelta * ((uwUserWeights >> 16) & 0xff);
  wDelta = GetComp(uwA, 1) - GetComp(uwB, 1);
  wDist += wDelta * wDelta * ((uwUserWeights >> 8) & 0xff);
  wDelta = GetComp(uwA, 0) - GetComp(uwB, 0);
  wDist += wDelta * wDelta * ((uwUserWeights >> 0) & 0xff);
  return (TDistance)wDist;
};

#define FIND_FAR(OFF)                                                          \
  if ((pSamples[aStartIdx + (OFF)] == aHead) && (wNewHeadIdx == -1))           \
    wNewHeadIdx = aStartIdx + (OFF);                                           \
  else if (lDistances[aStartIdx + (OFF)] >= lMaxDist)                          \
  lMaxDist = lDistances[aStartIdx + (OFF)], wFarthestIdx = aStartIdx + (OFF)

static inline void FindHeadAndFarthest(TDataFour *pSamples,
                                       TDistance *lDistances, int aStartIdx,
                                       int aLen, TDataFour aHead,
                                       int *pNewHeadIdx, int *pFarthestIdx) {
  int wNewHeadIdx = -1;
  int wFarthestIdx = -1;
  TDistance lMaxDist = 0;

  if (aLen > 1) {
    while (aLen >= 4) {
      FIND_FAR(0);
      FIND_FAR(1);
      FIND_FAR(2);
      FIND_FAR(3);
      aStartIdx += 4;
      aLen -= 4;
    }
    while (aLen > 0) {
      FIND_FAR(0);
      aStartIdx++;
      aLen--;
    }
  } else
    wNewHeadIdx = wFarthestIdx = aStartIdx;

  *pNewHeadIdx = wNewHeadIdx;
  *pFarthestIdx = wFarthestIdx;
}

// ---------------------------------------------------------------------------
// A DataFour is managed as a word of 32 bytes, in order [3:2:1:0]
static inline TDataFour DataFourMask(int aNumDimensions) {
  unsigned int uwMaskTab[5] = {0x0, 0xff, 0xffff, 0xffffff, 0xffffffff};
  ASSERT((aNumDimensions > 0) && (aNumDimensions < 5));
  return uwMaskTab[aNumDimensions];
}

// ---------------------------------------------------------------------------

static TDataFour AverageSamples(TDataFour *pSamples, int wNSamples) {
  TDataFour lAveraged = 0;
  int j, lD0, lD1, lD2, lD3;
  lD0 = lD1 = lD2 = lD3 = 0;
  for (j = 0; j < wNSamples; j++) {
    TDataFour lT = pSamples[j];
    lD3 += GetComp(lT, 3);
    lD2 += GetComp(lT, 2);
    lD1 += GetComp(lT, 1);
    lD0 += GetComp(lT, 0);
  }

  SetComp(&lAveraged, 3, lD3 / wNSamples);
  SetComp(&lAveraged, 2, lD2 / wNSamples);
  SetComp(&lAveraged, 1, lD1 / wNSamples);
  SetComp(&lAveraged, 0, lD0 / wNSamples);

  return lAveraged;
}

static inline int FindFarthest(TDataFour *pSamples, int wNSamples,
                               TDataFour aPoint, unsigned int uwUserWeights) {
  int j, lChosen = 0;
  TDistance lMaxDist = 0;
  for (j = 0; j < wNSamples; j++) {
    TDistance lDist = SquaredDistance(pSamples[j], aPoint, uwUserWeights);
    if (lDist >= lMaxDist)
      lMaxDist = lDist, lChosen = j;
  }
  return lChosen;
}

// Calculate bias depending on the quantity of samples falling on each side of
// the plane (unrolled)

static int CountToTheHead(TDataFour *aSrcSamples, TDistance *aSrcDistances,
                          TDistance *pTmpDistances, TDistance aFarthestSample,
                          int aStartIdx, int wNumSamples,
                          unsigned int uwUserWeights) {
  int wCountToTheHead = 0;
  while (wNumSamples >= 4) {
    pTmpDistances[aStartIdx + 0] = SquaredDistance(
        aFarthestSample, aSrcSamples[aStartIdx + 0], uwUserWeights);
    if (aSrcDistances[aStartIdx + 0] <= pTmpDistances[aStartIdx + 0])
      wCountToTheHead++;
    pTmpDistances[aStartIdx + 1] = SquaredDistance(
        aFarthestSample, aSrcSamples[aStartIdx + 1], uwUserWeights);
    if (aSrcDistances[aStartIdx + 1] <= pTmpDistances[aStartIdx + 1])
      wCountToTheHead++;
    pTmpDistances[aStartIdx + 2] = SquaredDistance(
        aFarthestSample, aSrcSamples[aStartIdx + 2], uwUserWeights);
    if (aSrcDistances[aStartIdx + 2] <= pTmpDistances[aStartIdx + 2])
      wCountToTheHead++;
    pTmpDistances[aStartIdx + 3] = SquaredDistance(
        aFarthestSample, aSrcSamples[aStartIdx + 3], uwUserWeights);
    if (aSrcDistances[aStartIdx + 3] <= pTmpDistances[aStartIdx + 3])
      wCountToTheHead++;
    aStartIdx += 4;
    wNumSamples -= 4;
  }
  while (wNumSamples > 0) {
    pTmpDistances[aStartIdx] =
        SquaredDistance(aFarthestSample, aSrcSamples[aStartIdx], uwUserWeights);
    if (aSrcDistances[aStartIdx] <= pTmpDistances[aStartIdx])
      wCountToTheHead++;
    aStartIdx++, wNumSamples--;
  }
  return wCountToTheHead;
}

// ---------------------------------------------------------------------------
// Takes a cluster and splits it in two, using a criterion of proximity from
// head or farthers point. The result is left in a second working buffer of
// clusters, in the same are butin different order This is the hard "kernel" of
// the alg., improvements should start from here
// ---------------------------------------------------------------------------

static void SplitByDistance(int wClusterNum, TWorkBuffers *pSrcBuffers,
                            TWorkBuffers *pDstBuffers, TDistance *pTmpDistances,
                            unsigned int uwUserWeights) {
  ASSERT(wClusterNum < pSrcBuffers->wNumClusters);

  int i;
  TCluster *pCluster = &pSrcBuffers->pClusters[wClusterNum];
  int wNumSamples = pCluster->wLenCluster;
  int wStartIdx = pCluster->wStartIdx;
  TDataFour *pSrcSamples = pSrcBuffers->pSamples;
  TDistance *pSrcDistances = pSrcBuffers->pDistancesToHead;
  int *pSrcOrigPos = pSrcBuffers->pOriginalPositions;
  TDataFour *pDstSamples = pDstBuffers->pSamples;
  TDistance *pDstDistances = pDstBuffers->pDistancesToHead;
  int *pDstOrigPos = pDstBuffers->pOriginalPositions;
  TDataFour uwFarthestSample = pSrcSamples[pCluster->wFarthestIdx];

  // Calculate bias depending on the quantity of samples falling on each side of
  // the plane
  int wCountToTheHead = 0;
  wCountToTheHead =
      CountToTheHead(pSrcSamples, pSrcDistances, pTmpDistances,
                     uwFarthestSample, wStartIdx, wNumSamples, uwUserWeights);

  int wBiasToTheHead = (wCountToTheHead * 0x100) / wNumSamples;
  if (wBiasToTheHead < 0x20)
    wBiasToTheHead = 0x20;
  if (wBiasToTheHead > 0xe0)
    wBiasToTheHead = 0xe0;

  // Efective split
  // These indexes are used to write the splitted results
  int wDstCloseToHeadIdx = wStartIdx;
  int wDstCloseToTailIdx = wStartIdx + wNumSamples;
  for (i = wStartIdx; i < (wStartIdx + wNumSamples); i++) {
    TDataFour uwSample = pSrcSamples[i];
    TDistance uwDistToHead = pSrcDistances[i];
    TDistance uwDistToTail = pTmpDistances[i];
    int wBranch = (uwDistToHead * wBiasToTheHead) <=
                  (uwDistToTail * (0x100 - wBiasToTheHead));
    wBranch = (uwDistToHead == uwDistToTail)
                  ? (i & 0x1)
                  : wBranch; // Trying to balance identical samples
    if (wBranch) {
      // "Closest to head" side of the split
      pDstSamples[wDstCloseToHeadIdx] = uwSample;
      pDstDistances[wDstCloseToHeadIdx] = uwDistToHead;
      pDstOrigPos[wDstCloseToHeadIdx] = pSrcOrigPos[i];
      wDstCloseToHeadIdx++;
    } else {
      // "Closest to tail" side of the split
      pDstSamples[wDstCloseToTailIdx - 1] = uwSample;
      pDstDistances[wDstCloseToTailIdx - 1] = uwDistToTail;
      pDstOrigPos[wDstCloseToTailIdx - 1] = pSrcOrigPos[i];
      wDstCloseToTailIdx--;
    }
  }
  // Register the new 2 clusters
  TCluster *lC0 = &pDstBuffers->pClusters[pDstBuffers->wNumClusters];
  TCluster *lC1 = &pDstBuffers->pClusters[pDstBuffers->wNumClusters + 1];
  pDstBuffers->wNumClusters += 2;

  // We need to find head and tail in the destination
  // Old heads are not useful as their position has changed
  lC0->wStartIdx = wStartIdx;
  lC0->wLenCluster = wDstCloseToHeadIdx - wStartIdx;
  FindHeadAndFarthest(pDstSamples, pDstDistances, lC0->wStartIdx,
                      lC0->wLenCluster, pSrcSamples[pCluster->wHeadIdx],
                      &lC0->wHeadIdx, &lC0->wFarthestIdx);

  lC1->wStartIdx = wDstCloseToTailIdx;
  lC1->wLenCluster = (wStartIdx + wNumSamples) - lC1->wStartIdx;
  FindHeadAndFarthest(pDstSamples, pDstDistances, lC1->wStartIdx,
                      lC1->wLenCluster, uwFarthestSample, &lC1->wHeadIdx,
                      &lC1->wFarthestIdx);
}

static inline void SwapWorkBuffers(AQuantizer *pQua) {
  TWorkBuffers *pTmp = pQua->pBuffersSrc;
  pQua->pBuffersSrc = pQua->pBuffersDst;
  pQua->pBuffersDst = pTmp;

  pQua->pBuffersDst->wNumClusters = 0; // Reset destination
}

// ---------------------------------------------------------------------------

static void CreateSeedCluster(AQuantizer *pQua, const TDataFour *pSamples,
                              int wNSamples) {
  int i;
  // - Choose a seed sample
  // - Mask the samples to remove possible scraps
  TDataFour uwMask = DataFourMask(4);
  for (i = 0; i < wNSamples; i++) {
    pQua->pBuffersSrc->pSamples[i] = pSamples[i] & uwMask;
    pQua->pBuffersSrc->pOriginalPositions[i] = i;
  }
  pQua->pBuffersSrc->pClusters->wStartIdx = 0;
  pQua->pBuffersSrc->pClusters->wLenCluster = wNSamples;

  // Find the best sample for heading the 1st cluster
  // A point in the extreme of the cluster seems to be the best head
  TDataFour lCentroid = AverageSamples(pQua->pBuffersSrc->pSamples, wNSamples);
  pQua->pBuffersSrc->pClusters->wHeadIdx = FindFarthest(
      pQua->pBuffersSrc->pSamples, wNSamples, lCentroid, pQua->uwUserWeights);

  // Find the farthest (from the seed) sample in this initial cluster
  TDistance lMaxDist = 0;
  for (i = 0; i < wNSamples; i++) {
    TDistance lDist = SquaredDistance(
        pQua->pBuffersSrc->pSamples[pQua->pBuffersSrc->pClusters->wHeadIdx],
        pQua->pBuffersSrc->pSamples[i], pQua->uwUserWeights);
    pQua->pBuffersSrc->pDistancesToHead[i] = lDist;
    if (lDist >= lMaxDist)
      lMaxDist = lDist, pQua->pBuffersSrc->pClusters->wFarthestIdx = i;
  }
  pQua->pBuffersSrc->wNumClusters = 1; // Created!
}

// ---------------------------------------------------------------------------

void ClusterSamples(AQuantizer *pQua, const TDataFour *pSamples,
                    int wNumSamples, int wNWantedClusters) {
  int j;
  ASSERT(wNWantedClusters >= 1);

  CreateSeedCluster(pQua, pSamples, wNumSamples);

  // This is the main loop of the quantizer
  while (pQua->pBuffersSrc->wNumClusters < wNWantedClusters) {
    int lNClusters = pQua->pBuffersSrc->wNumClusters;
    int lMaxSplits = wNWantedClusters - pQua->pBuffersSrc->wNumClusters;
    // Split clusters until we touch all the samples, or the number of
    // subsamples is reached
    while (lNClusters > 0) {
      int pClusterIdx = lNClusters - 1;
      if ((pQua->pBuffersSrc->pClusters[pClusterIdx].wLenCluster > 1) &&
          (lMaxSplits > 0)) {
        SplitByDistance(pClusterIdx, pQua->pBuffersSrc, pQua->pBuffersDst,
                        pQua->pTmpDistances, pQua->uwUserWeights);
        lMaxSplits--;
      } else {
        // If a cluster can't be splitted, it must be copied
        TCluster *lSrcCluster = &pQua->pBuffersSrc->pClusters[pClusterIdx];
        TCluster *lDstCluster =
            &pQua->pBuffersDst->pClusters[pQua->pBuffersDst->wNumClusters];
        if (lSrcCluster->wLenCluster == 1) {
          int lPos = lSrcCluster->wStartIdx;
          lDstCluster->wStartIdx = lDstCluster->wHeadIdx =
              lDstCluster->wFarthestIdx = lSrcCluster->wStartIdx;
          lDstCluster->wLenCluster = 1;
          pQua->pBuffersDst->pDistancesToHead[lPos] =
              pQua->pBuffersSrc->pDistancesToHead[lPos];
          pQua->pBuffersDst->pSamples[lPos] = pQua->pBuffersSrc->pSamples[lPos];
          pQua->pBuffersDst->pOriginalPositions[lPos] =
              pQua->pBuffersSrc->pOriginalPositions[lPos];
        } else {
          // Larger than 1 sample, and no split? We finished, so no
          // need to calculate farthest point or relocate the head
          lDstCluster->wStartIdx = lSrcCluster->wStartIdx;
          lDstCluster->wHeadIdx = lDstCluster->wFarthestIdx = -1;
          lDstCluster->wLenCluster = lSrcCluster->wLenCluster;
          for (j = lSrcCluster->wStartIdx;
               j < (lSrcCluster->wStartIdx + lSrcCluster->wLenCluster); j++) {
            pQua->pBuffersDst->pDistancesToHead[j] =
                pQua->pBuffersSrc->pDistancesToHead[j];
            pQua->pBuffersDst->pSamples[j] = pQua->pBuffersSrc->pSamples[j];
            pQua->pBuffersDst->pOriginalPositions[j] =
                pQua->pBuffersSrc->pOriginalPositions[j];
          }
        }
        pQua->pBuffersDst->wNumClusters++;
      }
      lNClusters--;
    }
    // Done; we swap the working buffers, so the result becomes the source for
    // the next iteration
    SwapWorkBuffers(pQua);
  }
}

// ---------------------------------------------------------------------------
// Time to collect the result. We obtain the centroids for all the resulting
// cluster That's the palette, or the result of subsampling...

static void SubsampleClusters(TWorkBuffers *pWb, TDataFour *pSubSamples) {
  int i;
  for (i = 0; i < pWb->wNumClusters; i++) {
    TCluster *pCluster = &pWb->pClusters[i];
    int lNSamples = pCluster->wLenCluster;
    pSubSamples[i] =
        AverageSamples(&pWb->pSamples[pCluster->wStartIdx], lNSamples);
  }
}

// ---------------------------------------------------------------------------
// PUBLIC
// ---------------------------------------------------------------------------

static void AllocateWorkBuffers(TWorkBuffers *pWb, int wMaxNumSubSamples,
                                int wMaxSamples) {
  pWb->pClusters = (TCluster *)malloc(sizeof(TCluster) * wMaxNumSubSamples);
  pWb->pSamples = (TDataFour *)malloc(sizeof(TDataFour) * wMaxSamples);
  pWb->pDistancesToHead = (TDistance *)malloc(sizeof(TDistance) * wMaxSamples);
  pWb->pOriginalPositions = (int *)malloc(sizeof(int) * wMaxSamples);
  pWb->wNumClusters = 0;
}

AQuantizer *Quantizer_Prepare(int wMaxSamples, int wMaxNumSubSamples,
                              int wNDimensions) {
  ASSERT((wNDimensions > 0) && (wNDimensions < 5));

  AQuantizer *pQua = (AQuantizer *)malloc(sizeof(AQuantizer));
  pQua->wNDimensions = wNDimensions;
  pQua->wMaxSamples = wMaxSamples;
  pQua->wMaxNumSubSamples = wMaxNumSubSamples;

  pQua->pTmpDistances = (TDistance *)malloc(sizeof(TDistance) * wMaxSamples);

  pQua->pBuffersSrc = (TWorkBuffers *)malloc(sizeof(TWorkBuffers));
  AllocateWorkBuffers(pQua->pBuffersSrc, wMaxNumSubSamples, wMaxSamples);
  pQua->pBuffersDst = (TWorkBuffers *)malloc(sizeof(TWorkBuffers));
  AllocateWorkBuffers(pQua->pBuffersDst, wMaxNumSubSamples, wMaxSamples);

  pQua->uwUserWeights = 0x01010101;
  return pQua;
}

static void DeleteWorkBuffers(TWorkBuffers *pWb) {
  free(pWb->pClusters);
  free(pWb->pSamples);
  free(pWb->pDistancesToHead);
  free(pWb->pOriginalPositions);
  free(pWb);
}

void Quantizer_Free(AQuantizer *pQua) {
  DeleteWorkBuffers(pQua->pBuffersSrc);
  DeleteWorkBuffers(pQua->pBuffersDst);
  free(pQua->pTmpDistances);
}

// ---------------------------------------------------------------------------

void Quantizer_SetWeights(AQuantizer *pQua, int wW0, int wW1, int wW2,
                          int wW3) {
  ASSERT((wW0 >= 0) && (wW0 < 256) && (wW1 >= 0) && (wW1 < 256) && (wW2 >= 0) &&
         (wW2 < 256) && (wW3 >= 0) && (wW3 < 256));
  pQua->uwUserWeights = (wW3 << 24) | (wW2 << 16) | (wW1 << 8) | wW0;
}

// ---------------------------------------------------------------------------

void QuantizeAndIndex8(AQuantizer *pQua, const TDataFour *pSamples,
                       int wNSamples, int wNSubSamples, unsigned char *pIndices,
                       TDataFour *pSubSamples) {
  int i, j;
  ClusterSamples(pQua, pSamples, wNSamples, wNSubSamples);
  SubsampleClusters(pQua->pBuffersSrc, pSubSamples);
  // Original position tracking replacer
  ASSERT(pQua->pBuffersSrc->wNumClusters <= pQua->wMaxNumSubSamples);
  for (i = 0; i < pQua->pBuffersSrc->wNumClusters; i++) {

    int lStart = pQua->pBuffersSrc->pClusters[i].wStartIdx;
    int lStop = lStart + pQua->pBuffersSrc->pClusters[i].wLenCluster;
    for (j = lStart; j < lStop; j++) {
      int lPos = pQua->pBuffersSrc->pOriginalPositions[j];
      pIndices[lPos] = i;
    }
  }
}

void QuantizeAndIndex16(AQuantizer *pQua, const TDataFour *pSamples,
                        int wNSamples, int wNSubSamples,
                        unsigned short *pIndices, TDataFour *pSubSamples) {
  int i, j;
  ClusterSamples(pQua, pSamples, wNSamples, wNSubSamples);
  SubsampleClusters(pQua->pBuffersSrc, pSubSamples);
  // Original position tracking replacer
  ASSERT(pQua->pBuffersSrc->wNumClusters <= pQua->wMaxNumSubSamples);
  for (i = 0; i < pQua->pBuffersSrc->wNumClusters; i++) {
    int lStart = pQua->pBuffersSrc->pClusters[i].wStartIdx;
    int lStop = lStart + pQua->pBuffersSrc->pClusters[i].wLenCluster;
    for (j = lStart; j < lStop; j++) {
      int lPos = pQua->pBuffersSrc->pOriginalPositions[j];
      pIndices[lPos] = i;
    }
  }
}
