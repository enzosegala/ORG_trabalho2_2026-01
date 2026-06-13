#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "config.h"
#include "astar.h"

uint64_t nodes_expanded = 0;
uint64_t heap_pushes = 0;
uint64_t heap_pops = 0;
uint64_t heuristic_calls = 0;

typedef struct {
    Node* data[MAX_OPEN_NODES];
    int size;
} MinHeap;

static Node nodes[GRID_HEIGHT][GRID_WIDTH];
static int map_grid[GRID_HEIGHT][GRID_WIDTH];

static float heuristic(int x1,int y1,int x2,int y2){
    heuristic_calls++;
    return (float)(abs(x1-x2)+abs(y1-y2));
}

static void heap_push(MinHeap* h, Node* n){
    int i=h->size++;
    heap_pushes++;
    while(i>0){
        int p=(i-1)/2;
        if(h->data[p]->f <= n->f) break;
        h->data[i]=h->data[p];
        i=p;
    }
    h->data[i]=n;
}

static Node* heap_pop(MinHeap* h){
    if(h->size==0) return NULL;
    heap_pops++;
    Node* root=h->data[0];
    Node* last=h->data[--h->size];

    int i=0;
    while(1){
        int l=2*i+1, r=2*i+2, s=i;
        if(l<h->size && h->data[l]->f < last->f) s=l;
        if(r<h->size && h->data[r]->f < h->data[s]->f) s=r;
        if(s==i) break;
        h->data[i]=h->data[s];
        i=s;
    }
    if(h->size) h->data[i]=last;
    return root;
}

static void generate_map(void){
    srand(RANDOM_SEED);
    for(int y=0;y<GRID_HEIGHT;y++){
        for(int x=0;x<GRID_WIDTH;x++){
            map_grid[y][x]=(rand()%100 < OBSTACLE_PERCENT);
        }
    }
    map_grid[0][0]=0;
    map_grid[GRID_HEIGHT-1][GRID_WIDTH-1]=0;
}

int main(void){
    generate_map();

    for(int y=0;y<GRID_HEIGHT;y++){
        for(int x=0;x<GRID_WIDTH;x++){
            nodes[y][x]=(Node){x,y,1e30f,0,0,-1,-1,0,0};
        }
    }

    MinHeap open={0};
    Node* start=&nodes[0][0];
    Node* goal=&nodes[GRID_HEIGHT-1][GRID_WIDTH-1];

    start->g=0;
    start->h=heuristic(0,0,goal->x,goal->y);
    start->f=start->h;

    heap_push(&open,start);

    const int dx[4]={1,-1,0,0};
    const int dy[4]={0,0,1,-1};

    while(open.size){
        Node* cur=heap_pop(&open);
        if(cur->closed) continue;

        cur->closed=1;
        nodes_expanded++;

        if(cur==goal) break;

        for(int k=0;k<4;k++){
            int nx=cur->x+dx[k];
            int ny=cur->y+dy[k];

            if(nx<0||ny<0||nx>=GRID_WIDTH||ny>=GRID_HEIGHT) continue;
            if(map_grid[ny][nx]) continue;

            Node* nb=&nodes[ny][nx];

            float ng=cur->g+1.0f;

            if(ng < nb->g){
                nb->g=ng;
                nb->h=HEURISTIC_WEIGHT *
                      heuristic(nx,ny,goal->x,goal->y);
                nb->f=nb->g+nb->h;
                nb->parent_x=cur->x;
                nb->parent_y=cur->y;
                heap_push(&open,nb);
            }
        }
    }

    printf("Nodes expanded : %llu\n",(unsigned long long)nodes_expanded);
    printf("Heap pushes    : %llu\n",(unsigned long long)heap_pushes);
    printf("Heap pops      : %llu\n",(unsigned long long)heap_pops);
    printf("Heuristic calls: %llu\n",(unsigned long long)heuristic_calls);

    return 0;
}
