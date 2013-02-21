#ifndef _BUDDY_H_
#define _BUDDY_H_

#ifdef NONKERNEL
#define bmalloc(...) malloc(__VA_ARGS__)
#else
#define bmalloc(...) vmalloc(__VA_ARGS__)
#endif

//#define POOL_SIZE   16777223
#define POOL_SIZE   16

typedef struct node_t node_t;

enum node_state {FREE, SPLIT, ALLOC};

struct node_t {
    int idx;
    enum node_state state;
    int size;
    node_t* left;
    node_t* right;
};

struct node_t *pool_head;

int buddy_alloc(node_t *n, int size);
int buddy_free(node_t *n, int idx);
int buddy_get_size(node_t *n, int idx);
void print_tree(node_t *n);

#endif
