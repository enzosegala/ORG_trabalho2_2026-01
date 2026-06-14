#ifndef FFT_H
#define FFT_H

#include <complex.h>

/*
 * Declaração da função FFT in-place Radix-2.
 * O vetor X deve possuir tamanho N, onde N é obrigatoriamente potência de 2.
 */
void fft(double complex *X, int N);

#endif
