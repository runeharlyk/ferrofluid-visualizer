#ifndef BIQUAD_H
#define BIQUAD_H

struct Biquad {
    float alpha1;
    float alpha2;

    float beta0;
    float beta1;
    float beta2;

    float w_1;
    float w_2;
    
    float output;
};

typedef struct Biquad Biquad;

void BiquadInit(Biquad *bq);

// Only to be called by other specific filter creator functions. Beware b before a.
void BiquadSetParams(Biquad *bq, float b0, 
                                 float b1, 
                                 float b2, 
                                 float a1,
                                 float a2);

float BiquadUpdate(Biquad *bq, float input);

#endif //BIQUAD_H