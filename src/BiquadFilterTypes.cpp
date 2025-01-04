#include <Arduino.h>
#include "BiquadFilterTypes.h"

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

BiquadCalcValues BiquadGetCommonValues(float Q, float fc, float Fs, float dbGain, bool shelf_or_peak) {
    BiquadCalcValues out;
    float omega = 2 * PI * (fc / Fs);
    out.cos_omega = cos(omega);
    out.sin_omega = sin(omega);

    out.alpha = out.sin_omega / (2*Q);

    if (shelf_or_peak == true) {
        out.A = pow(10, dbGain/40.0f);
    } else {
        out.A = 0.0f;
    }

    return out;
}

void BiquadLowpass(Biquad *bq, float Q, float fc, float Fs) {
    float a0, a1, a2, b0, b1, b2;
    
    BiquadCalcValues values = BiquadGetCommonValues(Q, fc, Fs, 0, false);
    
    a0 = 1.0f + values.alpha;
    a1 = -2.0f * values.cos_omega;
    a2 = 1.0f - values.alpha;

    b0 = (1.0f - values.cos_omega) / 2.0f;
    b1 = 1.0f - values.cos_omega;
    b2 = b0;

    // Normalize to a0 = 1;

    a1 = a1 / a0;
    a2 = a2 / a0;
    b0 = b0 / a0;
    b1 = b1 / a0;
    b2 = b2 / a0;

    BiquadSetParams(bq, b0, b1, b2, a1, a2);
}

void BiquadHighpass(Biquad *bq, float Q, float fc, float Fs) {

}
