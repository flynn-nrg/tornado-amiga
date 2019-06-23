#ifndef _QUANTIZER_H_
#define _QUANTIZER_H_

typedef unsigned int TDataFour;
typedef unsigned int TDistance;

struct SAQuantizer;
typedef struct SAQuantizer AQuantizer;

AQuantizer *Quantizer_Prepare(int wMaxSamples, int wMaxNumSubSamples,
                              int wDimensions);
void Quantizer_Free(AQuantizer *pQua);

void Quantizer_SetWeights(AQuantizer *pQua, int wW0, int wW1, int wW2, int wW3);

void QuantizeAndIndex8(AQuantizer *pQua, const TDataFour *pSamples,
                       int wNSamples, int wNSubSamples, unsigned char *pIndices,
                       TDataFour *pSubSamples);

void QuantizeAndIndex16(AQuantizer *pQua, const TDataFour *pSamples,
                        int wNSamples, int wNSubSamples,
                        unsigned short *pIndices, TDataFour *pSubSamples);

#endif // _QUANTIZER_H_
