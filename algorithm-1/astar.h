#ifndef ASTAR_H
#define ASTAR_H

#include <stdint.h>

typedef struct {
    int x, y;
    float g, h, f;
    int parent_x, parent_y;
    int opened, closed;
} Node;

extern uint64_t nodes_expanded;
extern uint64_t heap_pushes;
extern uint64_t heap_pops;
extern uint64_t heuristic_calls;

#endif
