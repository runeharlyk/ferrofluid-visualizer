#ifndef BIQUAD_FILTER_TYPES_H
#define BIQUAD_FILTER_TYPES_H

#include "Biquad.h"

struct BiquadCalcValues {
    float cos_omega;
    float sin_omega;
    float alpha;
    float A;
};

typedef struct BiquadCalcValues BiquadCalcValues;

BiquadCalcValues BiquadGetCommonValues(float Q, float fc, float Fs, float dbGain, bool shelf_or_peak);

// Biquad pointed to by first parameter must be initialized by calling BiquadInit() beforehand.
void BiquadLowpass(Biquad *bq, float Q, float fc, float Fs);
void BiquadHighpass(Biquad *bq, float Q, float fc, float Fs);



#endif //BIQUAD_FILTER_TYPES_H