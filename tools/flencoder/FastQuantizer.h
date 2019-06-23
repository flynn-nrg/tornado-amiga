
/*

Purpose:
    This quantizer does NOT support signed numbers,
    so all samples must be centered.
    It's a good algorithm for small palettes.
    It supports samples with 1 to 4 dimensions, but in packed format (int = 4
bytes) Memory for the palette and the indices must be reserved by the user of
the class Use: Instance once, and use it several times for optimum performance
(if possible) lQuantizer = MMIDQuantizer  ( 1000, 16, 3); for (...)
        QuantizeAndIndex ( lSamples, 1000, 16, lIndices, lSubSamples);
    delete lQuantizer;
Memory footprint:
    Bytes:  2 * ((24 * Max. N. samples) + (20 * Max. N. subsamples)) + (4 * Max.
N. samples) Performance cost: Constant * N. samples * (Sqrt(N. subsamples)/2)
    Cycles in ARM9: to be measured (Around 400 cicles / pixel)
    Number of dimensions has little impact, as the data structures remains the
same Based on: Implementation of "Minimizing the maximun intercluster distance"
    By Teofilo F. Gonzalez and Zhigang Xiang
TODO:
    The calculation of lBiasToTheHead is a hack, and probably sometimes
incorrect; should be improved Divisions should be removed
*/

#ifndef _QUANTIZER_H_
#define _QUANTIZER_H_

// The format for samples
// Up to 4, in byte format
typedef unsigned int TDataFour;
typedef unsigned int TDistance;

struct SAQuantizer;
typedef struct SAQuantizer AQuantizer;

// Create a quantizer instance

AQuantizer *Quantizer_Prepare(int wMaxSamples, int wMaxNumSubSamples,
                              int wDimensions);
void Quantizer_Free(AQuantizer *pQua);

//
void Quantizer_SetWeights(AQuantizer *pQua, int wW0, int wW1, int wW2, int wW3);

// Quantize a set
void QuantizeAndIndex8(AQuantizer *pQua, const TDataFour *pSamples,
                       int wNSamples, int wNSubSamples, unsigned char *pIndices,
                       TDataFour *pSubSamples);

void QuantizeAndIndex16(AQuantizer *pQua, const TDataFour *pSamples,
                        int wNSamples, int wNSubSamples,
                        unsigned short *pIndices, TDataFour *pSubSamples);

#endif // _QUANTIZER_H_
