/**
 * @file astar.h
 * @brief Definições de estruturas e protótipos para o algoritmo A*.
 *
 * Foco em eficiência de hardware: estruturas compactas para reduzir a pegada
 * de cache (cache footprint) e evitar o espalhamento de ponteiros (pointer chasing).
 */

#ifndef ASTAR_H
#define ASTAR_H

// Estrutura compacta (8 bytes). Passada por valor para caber diretamente em registradores (CPU registers).
typedef struct {
    int x;
    int y;
} Point;

// Nó da Fila de Prioridade (12 bytes com packing ideal). Alinhamento natural.
typedef struct {
    Point pt;
    float f_cost;
} HeapNode;

// Min-Heap baseada em array contíguo. Aloca memória em um único bloco estável.
// Evita chamadas repetidas a malloc() durante a busca, eliminando overhead do S.O.
typedef struct {
    HeapNode *data;
    int size;
    int capacity;
} MinHeap;

// Gerenciamento da Fila de Prioridade
MinHeap* createHeap(int capacity);
void freeHeap(MinHeap* heap);
void push(MinHeap* heap, Point pt, float f_cost);
HeapNode pop(MinHeap* heap);

// Heurísticas de Busca
float heuristic_manhattan(Point a, Point b);
float heuristic_euclidean(Point a, Point b);

// Função Principal de Execução
void astar(int* grid, int width, int height, Point start, Point goal, int heuristic_type);

#endif // ASTAR_H
