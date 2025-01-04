#include "Biquad.h"

void BiquadInit(Biquad *bq) {
    bq->alpha1 = 0.0f;
    bq->alpha2 = 0.0f;
    bq->beta0  = 0.0f;
    bq->beta1  = 0.0f;
    bq->beta2  = 0.0f;
    bq->w_1    = 0.0f;
    bq->w_2    = 0.0f;
    bq->output = 0.0f;
}

void BiquadSetParams(Biquad *bq, float b0, float b1, float b2, float a1, float a2) {
    
    float params[5] = {b0, b1, b2, a1, a2};
    
    // Clamp all parameters between 0 and 1
    /*
    for (int i = 0; i < 5; i++)
    {
        if (params[i] > 1.0f) {
            params[i] = 1.0f;
        } else if (params[i] < 0.0f) {
            params[i] = 0.0f;
        }
        
    }
    */
    
    bq->beta0  = params[0];
    bq->beta1  = params[1];
    bq->beta2  = params[2];
    bq->alpha1 = params[3];
    bq->alpha2 = params[4];

}

float BiquadUpdate(Biquad *bq, float input) {
    
    float wn;

    wn = input - (bq->alpha1 * bq->w_1) - (bq->alpha2 * bq->w_2);

    bq->output = (bq->beta0 * wn) + (bq->beta1 * bq->w_1) + (bq->beta2 * bq->w_2);

    bq->w_2 = bq->w_1;
    bq->w_1 = wn;

    return bq->output;
}
