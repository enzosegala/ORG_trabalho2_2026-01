#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* -------------------------------------------------- */
/* Constantes e limites                               */
/* -------------------------------------------------- */
#define MAX_NOS 5000
#define MAX_ARESTAS 16
#define INFINITO   1e30f    /* Representa "custo desconhecido"   */
#define NENHUM      -1      /* Sentinela: "nó inexistente"       */

/* -------------------------------------------------- */
/* Estruturas de dados                                */
/* -------------------------------------------------- */

/* Uma aresta conecta o nó atual a um vizinho com um custo */
typedef struct {
    int   destino;
    float custo;
} Aresta;

/* Cada nó tem posição no plano e uma lista de vizinhos */
typedef struct {
    float x, y;
    Aresta vizinhos[MAX_ARESTAS];
    int    num_vizinhos;
} No;

/* -------------------------------------------------- */
/* Grafo global                                       */
/* -------------------------------------------------- */

static No grafo[MAX_NOS];

/* -------------------------------------------------- */
/* Heurística: distância em linha reta até o destino */
/* -------------------------------------------------- */

static float heuristica(int atual, int destino)
{
    float dx = grafo[atual].x - grafo[destino].x;
    float dy = grafo[atual].y - grafo[destino].y;
    return sqrtf(dx * dx + dy * dy);
}

/* -------------------------------------------------- */
/* Adiciona uma aresta no grafo                       */
/* -------------------------------------------------- */

static void adicionar_aresta(int origem, int destino, float custo)
{
    if (grafo[origem].num_vizinhos >= MAX_ARESTAS)
        return; /* Lista cheia, ignora */

    int i = grafo[origem].num_vizinhos++;
    grafo[origem].vizinhos[i].destino = destino;
    grafo[origem].vizinhos[i].custo   = custo;
}
/* -------------------------------------------------- */
/* Gera um grafo pseudoaleatório moderadamente denso  */
/* -------------------------------------------------- */

static void gerar_grafo_grande(void)
{
    srand(42);

    for(int i = 0; i < MAX_NOS; i++)
    {
        grafo[i].num_vizinhos = 0;

        /* coordenadas para a heurística */
        grafo[i].x = (float)(rand() % 1000);
        grafo[i].y = (float)(rand() % 1000);
    }

    for(int i = 0; i < MAX_NOS; i++)
    {
        for(int k = 0; k < MAX_ARESTAS; k++)
        {
            int destino = rand() % MAX_NOS;

            if(destino == i)
                continue;

            float dx =
                grafo[i].x -
                grafo[destino].x;

            float dy =
                grafo[i].y -
                grafo[destino].y;

            float custo =
                sqrtf(dx*dx + dy*dy);

            adicionar_aresta(
                i,
                destino,
                custo);
        }
    }
}
/* -------------------------------------------------- */
/* A* principal                                       */
/* Retorna 1 se encontrou caminho, 0 caso contrário  */
/* -------------------------------------------------- */

static int astar(int inicio, int destino)
{
    /* g[n] = custo real do caminho percorrido até n */
    float g[MAX_NOS];

    /* f[n] = g[n] + heuristica(n, destino) */
    float f[MAX_NOS];

    /* anterior[n] = de qual nó chegamos a n (para reconstruir caminho) */
    int anterior[MAX_NOS];

    /* aberto[n]   = nó ainda a explorar */
    /* fechado[n]  = nó já explorado, não revisitamos */
    int aberto[MAX_NOS];
    int fechado[MAX_NOS];
    int num_abertos = 0;

    /* Inicializa todos os custos como infinito */
    for (int i = 0; i < MAX_NOS; i++) {
        g[i]        = INFINITO;
        f[i]        = INFINITO;
        anterior[i] = NENHUM;
        aberto[i]   = 0;
        fechado[i]  = 0;
    }

    /* O nó inicial tem custo zero */
    g[inicio] = 0.0f;
    f[inicio] = heuristica(inicio, destino);

    /* Coloca o nó inicial na lista de abertos */
    aberto[num_abertos++] = inicio;

    /* ---- Loop principal do A* ---- */
    while (num_abertos > 0)
    {
        /* 1) Escolhe o nó com menor f na lista de abertos */
        float menor_f   = INFINITO;
        int   idx_atual = NENHUM;

        for (int i = 0; i < num_abertos; i++) {
            int no = aberto[i];
            if (f[no] < menor_f) {
                menor_f   = f[no];
                idx_atual = i;
            }
        }

        int atual = aberto[idx_atual];

        /* 2) Remove o nó atual da lista de abertos */
        aberto[idx_atual] = aberto[--num_abertos];

        /* 3) Chegamos ao destino? */
        if (atual == destino) {
         //   printf("Caminho encontrado! Custo total: %.2f\n", g[destino]);

            /* Reconstrói e imprime o caminho de trás pra frente */
          //  printf("Caminho: ");
            int passo = destino;
            int pilha[MAX_NOS];
            int tam = 0;
            while (passo != NENHUM) {
                pilha[tam++] = passo;
                passo = anterior[passo];
            }
            for (int i = tam - 1; i >= 0; i--)
            //    printf("%d%s", pilha[i], i > 0 ? " -> " : "\n");

            return 1;
        }

        /* 4) Marca o nó atual como fechado (já explorado) */
        fechado[atual] = 1;

        /* 5) Examina cada vizinho do nó atual */
        for (int e = 0; e < grafo[atual].num_vizinhos; e++)
        {
            int   viz   = grafo[atual].vizinhos[e].destino;
            float custo = grafo[atual].vizinhos[e].custo;

            /* Ignora vizinhos já explorados */
            if (fechado[viz])
                continue;

            /* Custo tentativo para chegar ao vizinho passando pelo nó atual */
            float g_tentativo = g[atual] + custo;

            /* Se encontramos um caminho mais barato, atualiza */
            if (g_tentativo < g[viz]) {
                g[viz]        = g_tentativo;
                f[viz]        = g_tentativo + heuristica(viz, destino);
                anterior[viz] = atual;

                /* Adiciona à lista de abertos se ainda não estiver */
                int ja_esta = 0;
                for (int i = 0; i < num_abertos; i++) {
                    if (aberto[i] == viz) { ja_esta = 1; break; }
                }
                if (!ja_esta)
                    aberto[num_abertos++] = viz;
            }
        }
    }

    /* Lista de abertos esgotou: não existe caminho */
    //printf("Nenhum caminho encontrado entre %d e %d.\n", inicio, destino);
    return 0;
}

/* -------------------------------------------------- */
/* Exemplo de uso                                     */
/* -------------------------------------------------- */

int main(void)
{
    /* Build a large connected graph */
    gerar_grafo_grande();

    volatile int resultado = 0;
    for(int i = 0; i < 10; i++)
    {
        resultado += astar(
            (i * 137) % MAX_NOS,
            (i * 997) % MAX_NOS);
    }

    return resultado;
}