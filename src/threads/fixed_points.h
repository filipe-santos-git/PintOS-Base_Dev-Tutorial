#ifndef FIXED_POINTS_H
#define FIXED_POINTS_H

#include <stdint.h>

/* Biblioteca de aritmética em ponto fixo criada para o MLFQS.
   O kernel PintOS não usa ponto flutuante (FPU não é salva no contexto
   de interrupção), então representamos decimais como inteiros de 32 bits
   onde os 14 bits menos significativos são a parte fracionária.

   FP_SCALE = 2^14 = 16384. Exemplo: 1.5 é armazenado como 1.5*16384 = 24576. */

#define FP_SCALE (1<<14) 
typedef int fixed_t;

/* Converte inteiro n para ponto fixo: n * 2^14 */
static inline fixed_t int_to_fp(int n){ return n * FP_SCALE; }

/* Converte ponto fixo para inteiro com arredondamento ao mais próximo. */
static inline int fp_to_int_rounded(fixed_t n){
    if(n >= 0) return (n + FP_SCALE / 2) / FP_SCALE;
    return (n - FP_SCALE / 2) / FP_SCALE;
}

/* Soma ponto fixo com inteiro: converte b antes de somar. */
static inline fixed_t add_fp_int(fixed_t a, int b){ return a + int_to_fp(b); }

/* Soma dois valores em ponto fixo: escala já é a mesma, soma direta. */
static inline fixed_t add_fp(fixed_t a, fixed_t b){ return a + b; }

/* Subtrai dois valores em ponto fixo. */
static inline fixed_t sub_fp(fixed_t a, fixed_t b){ return a - b; }

/* Multiplica dois ponto fixo: produto intermediário em int64 para evitar
   overflow, depois divide por FP_SCALE para corrigir a escala duplicada. */
static inline fixed_t mult_fp(fixed_t a, fixed_t b){
    return ((int64_t) a) * b / FP_SCALE;
}

/* Multiplica ponto fixo por inteiro: inteiro não afeta a escala. */
static inline fixed_t mult_fp_int(fixed_t a, int b){
    return a * b;
}

/* Divide dois ponto fixo: multiplica a por FP_SCALE antes de dividir
   para compensar a perda de escala. Usa int64 para evitar overflow. */
static inline fixed_t div_fp(fixed_t a, fixed_t b){
    return ((int64_t) a) * FP_SCALE / b;
}

/* Divide ponto fixo por inteiro: inteiro não afeta a escala. */
static inline fixed_t div_fp_int(fixed_t a, int b){
    return a / b;
}

#endif
