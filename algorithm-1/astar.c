/**
 * PROGRAMA 1 -> ALGORITMO A*
 *
 * Analisando o Hardware:
 * 1. Localidade Espacial: O grid 2D é linearizado em um vetor 1D. Linhas de cache (Cache Lines de 64 bytes)
 * são carregadas de forma previsível pelo Hardware Prefetcher da CPU.
 * 2. Min-Heap Contígua: Em vez de árvores ligadas por ponteiros (que causam cache thrashing),
 * usamos indexação matemática clássica (2*i + 1) em memória contígua.
 * 3. Branch Prediction: Loops de bubble-up e bubble-down contêm condicionais desafiadoras para o
 * preditor de saltos da CPU. Otimizações de compilação (-O3) tentam usar instruções CMOV (Conditional Move).
 */

#include "astar.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

// Aloca a heap em um bloco contíguo único
MinHeap* createHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!heap) return NULL;
    heap->capacity = capacity;
    heap->size = 0;
    // Detalhe de Hardware: Alocação única garante que os nós da heap fiquem próximos em RAM
    heap->data = (HeapNode*)malloc(sizeof(HeapNode) * capacity);
    return heap;
}

void freeHeap(MinHeap* heap) {
    if (heap) {
        free(heap->data);
        free(heap);
    }
}

static inline void swap(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

// Inserção O(log N). Estressa o Branch Predictor devido ao laço condicional dependente de dados.
void push(MinHeap* heap, Point pt, float f_cost) {
    if (heap->size == heap->capacity) return;
    
    int i = heap->size++;
    heap->data[i] = (HeapNode){pt, f_cost};
    
    // Bubble up: Ajusta a árvore subindo o nó
    while (i != 0 && heap->data[(i - 1) / 2].f_cost > heap->data[i].f_cost) {
        swap(&heap->data[i], &heap->data[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

// Extração do menor elemento O(log N)
HeapNode pop(MinHeap* heap) {
    if (heap->size <= 0) return (HeapNode){{-1, -1}, FLT_MAX};
    if (heap->size == 1) return heap->data[--heap->size];
    
    HeapNode root = heap->data[0];
    heap->data[0] = heap->data[--heap->size];
    
    int i = 0;
    // Bubble down: Rebalanceia a heap a partir do topo
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        
        if (left < heap->size && heap->data[left].f_cost < heap->data[smallest].f_cost) {
            smallest = left;
        }
        if (right < heap->size && heap->data[right].f_cost < heap->data[smallest].f_cost) {
            smallest = right;
        }
        
        if (smallest != i) {
            swap(&heap->data[i], &heap->data[smallest]);
            i = smallest;
        } else {
            break;
        }
    }
    return root;
}

// Heurística Manhattan: Apenas operações inteiras simples (Absoluto e Adição). Levissima para a ALU.
float heuristic_manhattan(Point a, Point b) {
    return (float)(abs(a.x - b.x) + abs(a.y - b.y));
}

// Heurística Euclidiana: Usa sqrtf e multiplicações de ponto flutuante. 
// Estressa a unidade FPU/AVX da CPU. Sqrt é uma instrução de alta latência de hardware.
float heuristic_euclidean(Point a, Point b) {
    float dx = (float)(a.x - b.x);
    float dy = (float)(a.y - b.y);
    return sqrtf(dx * dx + dy * dy);
}

void astar(int* grid, int width, int height, Point start, Point goal, int heuristic_type) {
    int total_nodes = width * height;
    
    // Mitigação de Cache Misses: Arrays paralelos contíguos indexados por (y * width + x).
    // Evita estruturas encadeadas complexas e maximiza o aproveitamento das linhas de cache.
    float* g_cost = (float*)malloc(sizeof(float) * total_nodes);
    Point* parent = (Point*)malloc(sizeof(Point) * total_nodes);
    
    // Inicialização linear em memória (Amigável ao Write-Combining buffer do processador)
    for (int i = 0; i < total_nodes; i++) {
        g_cost[i] = FLT_MAX;
        parent[i] = (Point){-1, -1};
    }
    
    MinHeap* open_set = createHeap(total_nodes);
    if (!open_set || !g_cost || !parent) {
        printf("Falha de alocação críitica de memória.\n");
        return;
    }
    
    int start_idx = start.y * width + start.x;
    g_cost[start_idx] = 0.0f;
    push(open_set, start, 0.0f);
    
    // Vetores de direção para movimentos ortogonais (4 direções)
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    
    int nodes_expanded = 0;
    int found = 0;

    while (open_set->size > 0) {
        HeapNode current_node = pop(open_set);
        Point current = current_node.pt;
        
        // Destino alcançado
        if (current.x == goal.x && current.y == goal.y) {
            found = 1;
            break;
        }
        
        nodes_expanded++;
        int curr_idx = current.y * width + current.x;
        
        // Loop de vizinhos: Estressa o pipeline de execução da CPU com ramificações (Branches)
        for (int i = 0; i < 4; i++) {
            Point neighbor = {current.x + dx[i], current.y + dy[i]};
            
            // Branch Checkers: Condicionais de contorno que testam a especulação de execução da CPU
            if (neighbor.x < 0 || neighbor.x >= width || neighbor.y < 0 || neighbor.y >= height) continue;
            
            int neigh_idx = neighbor.y * width + neighbor.x;
            if (grid[neigh_idx] == 1) continue; // Obstáculo detectado
            
            float tentative_g = g_cost[curr_idx] + 1.0f;
            
            if (tentative_g < g_cost[neigh_idx]) {
                parent[neigh_idx] = current;
                g_cost[neigh_idx] = tentative_g;
                
                float h = (heuristic_type == 1) ? 
                          heuristic_manhattan(neighbor, goal) : 
                          heuristic_euclidean(neighbor, goal);
                
                push(open_set, neighbor, tentative_g + h);
            }
        }
    }
    
    // Print final estruturado para o script de testes capturar via stdout
    if (found) {
        printf("RESULT: SUCCESS | Expanded: %d | ", nodes_expanded);
        // Backtracking do tamanho da rota encontrada
        Point curr = goal;
        int path_length = 0;
        while (curr.x != -1) {
            path_length++;
            curr = parent[curr.y * width + curr.x];
        }
        printf("PathLength: %d\n", path_length - 1);
    } else {
        printf("RESULT: FAILURE | Expanded: %d | PathLength: 0\n", nodes_expanded);
    }
    
    // Limpeza completa de recursos - Previne Memory Leaks
    free(g_cost);
    free(parent);
    freeHeap(open_set);
}

int main(int argc, char* argv[]) {
    if (argc < 8) {
        fprintf(stderr, "Uso: %s <w> <h> <start_x> <start_y> <goal_x> <goal_y> <heuristic_type_1_or_2>\n", argv[0]);
        return 1;
    }

    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    Point start = {atoi(argv[3]), atoi(argv[4])};
    Point goal = {atoi(argv[5]), atoi(argv[6])};
    int heur = atoi(argv[7]);

    int* grid = (int*)malloc(sizeof(int) * w * h);
    if (!grid) return 1;

    // Leitura em stream contínua via stdin (Eficiência de I/O buffers)
    for (int i = 0; i < w * h; i++) {
        if (scanf("%d", &grid[i]) != 1) {
            grid[i] = 0;
        }
    }

    astar(grid, w, h, start, goal, heur);
    
    free(grid);
    return 0;
}
