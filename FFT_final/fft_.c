/*
 * fft_optimized.c
 *
 * FFT Radix-2 DIT (Decimation In Time) — versão otimizada para gem5
 *
 * Melhorias em relação à versão recursiva original:
 *   1. Buffer pré-alocado     → elimina malloc/free dentro da recursão
 *   2. Algoritmo iterativo    → opera in-place, sem cópias even[]/odd[]
 *   3. Tabela de twiddle      → pré-calcula cos/sin fora do loop quente
 *
 * Compilar:
 *   gcc -O2 -o fft_optimized fft_optimized.c -lm
 *
 * Uso:
 *   ./fft_optimized <N>
 *   Exemplo: ./fft_optimized 65536
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define PI 3.14159265358979323846

/* =========================================================
 * ESTRUTURAS DE DADOS
 * ========================================================= */

/*
 * Complex — par (real, imag) em AOS (Array of Structures).
 * Os dois campos são sempre acessados juntos nas operações
 * aritméticas, então AOS é adequado aqui.
 * Tamanho: 16 bytes (dois double de 8 bytes cada).
 */
typedef struct {
    double real;
    double imag;
} Complex;

/*
 * FFTPlan — estrutura que encapsula tudo que é pré-computado
 * antes da execução da FFT.
 *
 * Motivação: centraliza as alocações em um único ponto,
 * evitando fragmentação de heap e permitindo reutilização
 * do plano para múltiplas execuções com o mesmo N.
 */
typedef struct {
    int      N;        /* tamanho do sinal (deve ser potência de 2) */
    Complex *twiddle;  /* tabela de fatores de rotação W_N^k        */
} FFTPlan;


/* =========================================================
 * ARITMÉTICA COMPLEXA
 * ========================================================= */

/* Retorna a + b */
static inline Complex cadd(Complex a, Complex b)
{
    return (Complex){ a.real + b.real, a.imag + b.imag };
}

/* Retorna a - b */
static inline Complex csub(Complex a, Complex b)
{
    return (Complex){ a.real - b.real, a.imag - b.imag };
}

/*
 * Retorna a * b usando a fórmula:
 *   (ac - bd) + i(ad + bc)
 * onde a = ac + i*ad e b = bc + i*bd
 */
static inline Complex cmul(Complex a, Complex b)
{
    return (Complex){
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    };
}


/* =========================================================
 * MELHORIA 1 — ELIMINAÇÃO DE malloc/free DENTRO DA RECURSÃO
 *
 * Problema original:
 *   A cada chamada fft(x, N), eram feitos 2 mallocs de N/2
 *   elementos. Para N=65536, isso gera ~130 mil chamadas ao
 *   alocador, fragmentando o heap e tornando os acessos
 *   imprevisíveis para o prefetcher da L1.
 *
 * Solução:
 *   A melhoria 2 (iterativa in-place) elimina completamente
 *   a necessidade de buffers auxiliares. O único malloc
 *   necessário é o da tabela de twiddle (melhoria 3).
 *   Esta seção documenta a mudança arquitetural.
 * ========================================================= */


/* =========================================================
 * MELHORIA 2 — ALGORITMO ITERATIVO IN-PLACE (BIT-REVERSAL)
 *
 * Problema original:
 *   A versão recursiva separava x[] em even[] e odd[] a cada
 *   nível, copiando N/2 elementos duas vezes. O working set
 *   efetivo era o dobro do necessário, causando muitos misses
 *   na L1 para N grande.
 *
 * Solução:
 *   A FFT iterativa Cooley-Tukey opera diretamente em x[]
 *   sem nenhuma cópia. A separação even/odd é substituída
 *   por uma reordenação inicial por bit-reversal, seguida de
 *   log2(N) passagens de borboletas in-place.
 * ========================================================= */

/*
 * is_power_of_two — verifica se N é potência de 2.
 * A FFT Radix-2 exige essa condição.
 */
static int is_power_of_two(int N)
{
    return (N > 0) && ((N & (N - 1)) == 0);
}

/*
 * bit_reverse_permutation — reordena x[] pela permutação
 * de bit-reversal.
 *
 * A FFT DIT (Decimation In Time) exige que o array esteja
 * na ordem bit-reversed antes das borboletas. Por exemplo,
 * para N=8 (3 bits): índice 3 (011) vai para posição 6 (110).
 *
 * Acesso à memória: dois índices trocados por vez, O(N log N)
 * operações, todas in-place.
 */
static void bit_reverse_permutation(Complex *x, int N)
{
    int bits = (int)round(log2(N)); /* número de bits do índice */

    for (int i = 0; i < N; i++) {
        /* calcula o bit-reversal de i */
        int j = 0;
        for (int b = 0; b < bits; b++) {
            j |= ((i >> b) & 1) << (bits - 1 - b);
        }

        /* troca apenas uma vez (quando i < j) para evitar
         * desfazer a troca já realizada */
        if (i < j) {
            Complex tmp = x[i];
            x[i]        = x[j];
            x[j]        = tmp;
        }
    }
}


/* =========================================================
 * MELHORIA 3 — TABELA DE TWIDDLE FACTORS PRÉ-CALCULADA
 *
 * Problema original:
 *   cos() e sin() eram chamados dentro do loop interno da
 *   borboleta, a cada iteração. São funções caras (~20-100
 *   ciclos cada) e os valores se repetiam entre chamadas.
 *
 * Solução:
 *   Pré-calcula W_N^k = e^{-i 2π k/N} para k = 0..N/2-1
 *   em um array contíguo. Durante a execução, os twiddle
 *   factors são lidos com stride previsível, permitindo
 *   que o prefetcher da L1 os antecipe.
 *
 * Tamanho da tabela: (N/2) * 16 bytes
 *   N=1024  →  8 KB   (cabe inteira na L1 de 8 KB, no limite)
 *   N=8192  →  64 KB  (precisa de L1 >= 64 KB ou L2)
 *   N=65536 →  512 KB (fica na L2/L3)
 * ========================================================= */

/*
 * fft_create_plan — aloca e inicializa o plano de FFT.
 *
 * Retorna um FFTPlan pronto para uso com fft_exec().
 * Deve ser destruído com fft_destroy_plan() ao final.
 *
 * Custo: O(N) em tempo e memória, executado UMA vez.
 */
FFTPlan fft_create_plan(int N)
{
    FFTPlan p;
    p.N = N;

    /*
     * Aloca a tabela de N/2 twiddle factors.
     * Uma única alocação contígua garante boa localidade
     * espacial durante a execução.
     */
    p.twiddle = (Complex *)malloc((N / 2) * sizeof(Complex));
    if (!p.twiddle) {
        fprintf(stderr, "Erro: falha ao alocar tabela de twiddle (N=%d)\n", N);
        exit(EXIT_FAILURE);
    }

    /*
     * Pré-calcula W_N^k = cos(-2πk/N) + i*sin(-2πk/N)
     * para k = 0, 1, ..., N/2 - 1.
     *
     * O sinal negativo corresponde à DFT direta (análise).
     * Para IDFT (síntese), trocar o sinal.
     */
    for (int k = 0; k < N / 2; k++) {
        double angle    = -2.0 * PI * k / N;
        p.twiddle[k].real = cos(angle);
        p.twiddle[k].imag = sin(angle);
    }

    return p;
}

/*
 * fft_destroy_plan — libera a memória do plano.
 */
void fft_destroy_plan(FFTPlan *p)
{
    free(p->twiddle);
    p->twiddle = NULL;
    p->N       = 0;
}


/* =========================================================
 * EXECUÇÃO DA FFT — ITERATIVA IN-PLACE
 * ========================================================= */

/*
 * fft_exec — executa a FFT sobre o array x[] de tamanho p->N.
 *
 * O resultado substitui x[] in-place (X[k] no lugar de x[n]).
 *
 * Algoritmo: Cooley-Tukey iterativo DIT Radix-2
 *
 * Estrutura de acesso à memória:
 *   - Passo 1 (bit-reversal): acesso aleatório, mas O(N log N)
 *     swaps — feito apenas uma vez.
 *   - Passo 2 (borboletas): para cada nível len, percorre x[]
 *     em blocos de tamanho len com stride 1 dentro do bloco.
 *     Padrão sequencial → alta reutilização de linhas de cache.
 *
 * Complexidade: O(N log N) em tempo, O(1) em espaço auxiliar
 * (além do próprio x[] e da tabela de twiddle).
 */
void fft_exec(Complex *x, FFTPlan *p)
{
    int N = p->N;

    /* --- Passo 1: reordena x[] por bit-reversal ---
     *
     * Substitui a separação recursiva em even[]/odd[].
     * Após este passo, x[] está na ordem correta para
     * as borboletas bottom-up.
     */
    bit_reverse_permutation(x, N);

    /* --- Passo 2: borboletas iterativas bottom-up ---
     *
     * Cada iteração do loop externo corresponde a um
     * nível da árvore de recursão (de baixo para cima).
     * len vai de 2 (folhas) até N (raiz).
     */
    for (int len = 2; len <= N; len *= 2) {

        /*
         * step é o stride de acesso à tabela de twiddle.
         * Para um nível com grupos de tamanho len, os
         * twiddle factors são W_N^{0}, W_N^{step}, W_N^{2*step}...
         * que correspondem a W_len^{0}, W_len^{1}, W_len^{2}...
         *
         * Exemplo: N=8, len=4 → step=2
         *   usa twiddle[0], twiddle[2] (= W_8^0, W_8^2 = W_4^0, W_4^1)
         */
        int step = N / len;

        /*
         * Loop sobre os grupos (blocos de tamanho len no array).
         * Cada grupo é processado independentemente — boa
         * localidade espacial pois acessa len elementos contíguos.
         */
        for (int i = 0; i < N; i += len) {

            /*
             * Loop da borboleta dentro do grupo.
             * Acessa pares (x[i+k], x[i+k+len/2]) in-place.
             *
             * Borboleta DIT:
             *   t          = W * x[i+k+len/2]   (metade de baixo)
             *   x[i+k]     = x[i+k] + t          (soma)
             *   x[i+k+len/2] = x[i+k] - t        (diferença)
             */
            for (int k = 0; k < len / 2; k++) {

                /* lê twiddle factor pré-calculado com stride previsível */
                Complex w = p->twiddle[k * step];

                /* x[i+k+len/2] é a "metade de baixo" do grupo */
                Complex t = cmul(w, x[i + k + len / 2]);

                /* borboleta in-place */
                x[i + k + len / 2] = csub(x[i + k], t);
                x[i + k]           = cadd(x[i + k], t);
            }
        }
    }
    /* x[] agora contém X[k] = DFT{x[n]} para k = 0..N-1 */
}


/* =========================================================
 * FUNÇÕES AUXILIARES
 * ========================================================= */

/*
 * generate_signal — preenche x[] com um sinal de teste:
 * soma de duas senóides com frequências f1 e f2.
 *
 * Usado para verificar corretude: após a FFT, os picos
 * devem aparecer nos bins k = f1 e k = f2.
 */
static void generate_harmonic_signal(Complex *x, int N, int f1, int f2)
{
    for (int n = 0; n < N; n++) {
        x[n].real = cos(2.0 * PI * f1 * n / N)
                  + 0.5 * cos(2.0 * PI * f2 * n / N);
        x[n].imag = 0.0;
    }
}
static void dirac_delta(Complex *x, int N, int f1, int f2)
{
    for (int n = 0; n < N; n++) {
        x[n].real = 10000;
        x[n].imag = 0.0;
    }
}

static void generate_signal(Complex *x, int N, int f1, int f2)
{
    if (f1 == 0 && f2 == 0) {
        dirac_delta(x, N, f1, f2);
    }
    else if (f1 == 0) {
        for (int n = 0; n < N; n++) {
            x[n].real = 10000 + 0.5 * cos(2.0 * PI * f2 * n / N);
            x[n].imag = 32.0;
        }
    }
    else { 
        generate_harmonic_signal(x, N, f1, f2);
    }
}


void save_complex_array(const char *filename, const Complex *x, int n)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen");
        return;
    }

    for (int i = 0; i < n; i++) {
        fprintf(fp, "%.17g %.17g\n", x[i].real, x[i].imag);
    }

    fclose(fp);
}
/* =========================================================
 * MAIN
 * ========================================================= */

int main()
{

    int f1, f2;
    int N = 1024; 
    // // teste de N = 2**x
    // if (!is_power_of_two(N) || N < 2) {
    //     fprintf(stderr, "Erro: N=%d não é potência de 2 ou é menor que 2\n", N);
    //     return EXIT_FAILURE;
    // }
    Complex *x = (Complex *)malloc(N * sizeof(Complex)); 
    if (!x) {
        fprintf(stderr, "Erro: falha ao alocar sinal\n");
        return EXIT_FAILURE;
    }
    f1 = 289;
    f2 = 50;
    generate_signal(x, N, f1, f2); 
    FFTPlan plan = fft_create_plan(N);
    fft_exec(x, &plan); 
    /* --- Liberação de memória --- */
    fft_destroy_plan(&plan);
    free(x);

    return EXIT_SUCCESS;
}




// other cases     /////*
    //                     /////// EXECUCAO 1 
                
    //             f1 = 0;  /* frequência 1: bin DC (0 Hz) */
    //             f2 = 0; /* frequência 2: bin N/4  =*/ 
    //             //case 1 = dirac delta
    //             generate_signal(x, N, f1, f2); 
    //             printf("\nSinal de teste: f1=%d Hz, f2=%d Hz\n", f1, f2); 

    //             /* --- Criação do plano (pré-computa twiddle factors) ---
    //             *
    //             * Esta é a única alocação de heap além do próprio sinal.
    //             * Custo: O(N) em tempo e (N/2)*16 bytes de memória.
    //             */
    //             FFTPlan plan = fft_create_plan(N);
    //             printf("Plano criado. Executando FFT...\n");



    //             fft_exec(x, &plan);  /* resultado in-place em x[] */

    //             save_complex_array("fft_output1.txt", x, N);
    //             printf("FFT concluída. Saída salva em 'fft_output1.txt'\n");

    //             ///   EXECUSAO 2 

    //                     //x[n].real = 10000 + 0.5 * cos(2.0 * PI * f2 * n / N);
    //                 // x[n].imag = 32.0;
    //             f1 = 0;  /* frequência 1: bin DC (0 Hz) */
    //             f2 = 50; /* frequência 2: bin N/4  =*/ 
    //             //case 1 = dirac delta
    //             generate_signal(x, N, f1, f2); 
    //             printf("\nSinal de teste: f1=%d Hz, f2=%d Hz\n", f1, f2);



    //             fft_exec(x, &plan);  /* resultado in-place em x[] */

    //             save_complex_array("fft_output2.txt", x, N);
    //             printf("FFT concluída. Saída salva em 'fft_output2.txt'\n");


    // */////