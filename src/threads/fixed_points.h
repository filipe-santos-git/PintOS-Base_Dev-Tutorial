#ifndef FIXED_POINTS_H
#define FIXED_POINTS_H

#include <stdint.h>

/*
 * NOVO: Biblioteca de aritmética em ponto fixo para o MLFQS.
 *
 * O kernel PintOS não usa ponto flutuante (float/double) — operações de
 * FPU não são salvas no contexto de interrupção, portanto são proibidas
 * no kernel. Para calcular load_avg, recent_cpu e prioridade com decimais,
 * usamos ponto fixo: representamos um número real como um inteiro de 32 bits
 * onde os 14 bits menos significativos representam a parte fracionária.
 *
 * FP_SCALE = 2^14 = 16384
 *
 * Exemplo: o número real 1.5 é representado como 1.5 * 16384 = 24576.
 *
 * O tipo fixed_t é apenas um int (int32_t) com semântica de ponto fixo.
 * Operações entre dois fixed_t precisam de ajuste; operações com inteiros
 * simples (int) não precisam.
 */

#define FP_SCALE (1 << 14)   /* Fator de escala: 2^14 = 16384 */
typedef int fixed_t;         /* Tipo ponto fixo: inteiro de 32 bits escalado */

/*
 * Converte um inteiro n para ponto fixo.
 * Equivale a: n * 2^14
 * Ex: int_to_fp(1) = 16384, int_to_fp(2) = 32768
 */
static inline fixed_t int_to_fp(int n)
{
    return n * FP_SCALE;
}

/*
 * Converte um valor em ponto fixo para inteiro, com arredondamento.
 * Para n >= 0: arredonda para o inteiro mais próximo.
 * Para n <  0: arredonda em direção a zero (trunca para baixo em módulo).
 * Usado para expor recent_cpu e load_avg como inteiros às funções get_*.
 */
static inline int fp_to_int_rounded(fixed_t n)
{
    if (n >= 0) return (n + FP_SCALE / 2) / FP_SCALE;
    return (n - FP_SCALE / 2) / FP_SCALE;
}

/*
 * Soma um valor em ponto fixo (a) com um inteiro (b).
 * Como b é inteiro, precisa ser convertido antes de somar.
 * Resultado: a + b*FP_SCALE
 */
static inline fixed_t add_fp_int(fixed_t a, int b)
{
    return a + int_to_fp(b);
}

/*
 * Soma dois valores em ponto fixo.
 * Como ambos já estão escalados, a soma direta é correta.
 */
static inline fixed_t add_fp(fixed_t a, fixed_t b)
{
    return a + b;
}

/*
 * Subtrai dois valores em ponto fixo (a - b).
 */
static inline fixed_t sub_fp(fixed_t a, fixed_t b)
{
    return a - b;
}

/*
 * Multiplica dois valores em ponto fixo.
 *
 * Se fizéssemos só a * b, o resultado estaria escalado por FP_SCALE^2.
 * Portanto, dividimos por FP_SCALE para corrigir a escala.
 * Usamos int64_t no produto intermediário para evitar overflow de 32 bits.
 */
static inline fixed_t mult_fp(fixed_t a, fixed_t b)
{
    return ((int64_t) a) * b / FP_SCALE;
}

/*
 * Multiplica um valor em ponto fixo (a) por um inteiro (b).
 * Inteiro não altera a escala, basta multiplicar diretamente.
 */
static inline fixed_t mult_fp_int(fixed_t a, int b)
{
    return a * b;
}

/*
 * Divide dois valores em ponto fixo (a / b).
 *
 * Se fizéssemos só a / b, o resultado perderia a escala FP_SCALE.
 * Portanto, multiplicamos a por FP_SCALE antes de dividir.
 * Usamos int64_t para evitar overflow no produto intermediário.
 */
static inline fixed_t div_fp(fixed_t a, fixed_t b)
{
    return ((int64_t) a) * FP_SCALE / b;
}

/*
 * Divide um valor em ponto fixo (a) por um inteiro (b).
 * Inteiro não afeta a escala, basta dividir diretamente.
 */
static inline fixed_t div_fp_int(fixed_t a, int b)
{
    return a / b;
}

#endif /* FIXED_POINTS_H */
