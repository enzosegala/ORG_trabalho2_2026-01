#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include "fft.h"

#define PI 3.14159265358979323846

/* * ESTRESSE DE HARDWARE: Permutação de Reversão de Bits (Bit-Reversal Permutation)
 * Este passo rearranja o array de entrada trocando os elementos de lugar com base 
 * na inversão binária dos seus índices. É necessário para permitir a computação in-place.
 * * POR QUE ESTRESSA O HARDWARE: Os acessos à memória aqui são completamente não-lineares 
 * e não-sequenciais. Isso quebra totalmente a localidade espacial (Spatial Locality) 
 * que os algoritmos de prefetching de cache tentam prever. O processador sofre com constantes 
 * 'Cache Misses' nos níveis L1 e L2, invalidando linhas de cache e forçando stalls de execução 
 * enquanto espera barramentos buscarem dados na memória RAM principal. Para vetores muito grandes, 
 * saltar entre páginas de memória distantes também causa sobrecarga no TLB (Translation Lookaside Buffer).
 */
static void bit_reversal(double complex *X, int N) {
    int j = 0;
    for (int i = 0; i < N - 1; i++) {
        if (i < j) {
            double complex temp = X[i];
            X[i] = X[j];
            X[j] = temp;
        }
        int k = N / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }
}

/*
 * Algoritmo Cooley-Tukey Radix-2 Iterativo (In-Place)
 * Divide o problema de tamanho N em subproblemas log2(N) reduzindo a complexidade de O(N^2) para O(N log N).
 */
void fft(double complex *X, int N) {
    // Primeiro passo: rearranjo não-linear de memória
    bit_reversal(X, N);

    // Loop 1: Controla os estágios de agrupamento (profundidade da árvore = log2(N))
    for (int step = 2; step <= N; step *= 2) {
        int half_step = step / 2;
        
        /* * ESTRESSE DA FPU (Floating Point Unit) & PIPELINE:
         * Computamos o fator de rotação base (W_m) uma única vez por estágio através de cexp().
         * Invocar funções transcendentais complexas como cexp(), sin() ou cos() dentro dos loops mais internos 
         * congelaria o pipeline superescalar da CPU devido à alta latência desses cálculos aritméticos.
         * Em vez disso, mitigamos o gargalo calculando W_m na raiz e rotacionando o twiddle factor (W) 
         * de forma puramente multiplicativa (W *= W_m) dentro do loop interno, explorando instruções FMA 
         * (Fused Multiply-Add) de alta eficiência energética na FPU.
         */
        double complex W_m = cexp(-I * PI / half_step);

        // Loop 2: Avança entre os diferentes blocos independentes do estágio atual
        for (int i = 0; i < N; i += step) {
            double complex W = 1.0 + 0.0 * I;
            
            /* * Loop 3: Operação Borboleta (Butterfly Operation)
             * ESTRESSE DE MEMÓRIA (Stride & Bandwidth Saturation):
             * Aqui realizamos acessos simétricos em X[i + j] e X[i + j + half_step].
             * Nos estágios iniciais, half_step é pequeno (1, 2, 4...), de modo que ambos os elementos residem 
             * dentro da mesma linha de cache (Cache Line), maximizando o reuso dos dados.
             * Conforme o algoritmo avança, half_step cresce exponencialmente (N/4, N/2). O 'stride' (pulo) 
             * entre acessos ultrapassa a capacidade física dos caches. O hardware entra em estresse severo de 
             * largura de banda de memória (Memory Bandwidth), pois cada borboleta exige carregar e descarregar 
             * linhas de cache inteiramente separadas da RAM em paralelo, saturando os barramentos de interconexão.
             */
            for (int j = 0; j < half_step; j++) {
                double complex u = X[i + j];
                double complex t = W * X[i + j + half_step];
                
                // Cálculo aritmético centralizado da borboleta
                X[i + j] = u + t;
                X[i + j + half_step] = u - t;
                
                // Avança o ângulo do fator multiplicativo de rotação
                W *= W_m; 
            }
        }
    }
}

/*
 * FUNÇÃO MAIN: Gerencia a execução via ARGUMENTOS DE LINHA DE COMANDO
 * Uso esperado: ./fft_test <tamanho_N>
 * O programa espera receber N valores complexos formatados por linhas via entrada padrão (stdin).
 */
int main(int argc, char *argv[]) {
    // Validação dos argumentos mínimos obrigatórios
    if (argc < 2) {
        fprintf(stderr, "Erro de sintaxe. Uso correto: %s <tamanho_N>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 1024\n", argv[0]);
        return 1;
    }

    // Conversão do argumento textual para inteiro numérico
    int N = atoi(argv[1]);
    
    // Verificação matemática se N é uma potência exata de 2 positiva (Bitwise check: N & (N-1))
    if (N <= 0 || (N & (N - 1)) != 0) {
        fprintf(stderr, "Erro crítico: O tamanho N (%d) fornecido via argumento DEVE ser uma potência de 2 positiva (ex: 16, 128, 512, 4096).\n", N);
        return 1;
    }

    // Alocação contígua de memória física na Heap
    double complex *signal = malloc(N * sizeof(double complex));
    if (!signal) {
        fprintf(stderr, "Erro crítico: Falha catastrófica de alocação de memória para N = %d\n", N);
        return 1;
    }

    // Captura dos dados complexos alimentados via Stream Standard Input (stdin)
    double real, imag;
    for (int i = 0; i < N; i++) {
        if (scanf("%lf %lf", &real, &imag) != 2) {
            fprintf(stderr, "Erro de Input: Falha ao parsear o elemento índice %d (formato esperado: 'real imag').\n", i);
            free(signal);
            return 1;
        }
        signal[i] = real + imag * I;
    }

    // Processamento computacional da transformação de domínio
    fft(signal, N);

    // Impressão limpa dos resultados no Standard Output (stdout) para consumo do script de validação
    for (int i = 0; i < N; i++) {
        printf("%.10f %.10f\n", creal(signal[i]), cimag(signal[i]));
    }

    // Desalocação segura de ponteiros de memória
    free(signal);
    return 0;
}
