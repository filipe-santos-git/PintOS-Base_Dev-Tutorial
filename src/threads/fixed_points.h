#ifndef FIXED_POINTS_H
#define FIXED_POINTS_H

#include <stdint.h>

#define FP_SCALE (1<<14) 
typedef int fixed_t;

static inline fixed_t int_to_fp(int n){ return n * FP_SCALE; }

static inline int fp_to_int_rounded(fixed_t n){
    if(n >= 0) return (n + FP_SCALE / 2) / FP_SCALE;
    return (n - FP_SCALE / 2) / FP_SCALE;
}

static inline fixed_t add_fp_int(fixed_t a, int b){ return a + int_to_fp(b); }

static inline fixed_t add_fp(fixed_t a, fixed_t b){ return a + b; }

static inline fixed_t sub_fp(fixed_t a, fixed_t b){ return a - b; }


static inline fixed_t mult_fp(fixed_t a, fixed_t b){
    return ((int64_t) a) * b / FP_SCALE;
}

static inline fixed_t mult_fp_int(fixed_t a, int b){
    return a * b;
}

static inline fixed_t div_fp(fixed_t a, fixed_t b){
    return ((int64_t) a) * FP_SCALE / b;
}
static inline fixed_t div_fp_int(fixed_t a, int b){
    return a / b;
}

#endif