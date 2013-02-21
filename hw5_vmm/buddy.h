#ifndef _BUDDY_H_
#define _BUDDY_H_

#ifdef NONKERNEL
#define bmalloc(...) malloc(__VA_ARGS__)
#else
#define bmalloc(...) vmalloc(__VA_ARGS__)
#endif

//#define POOL_SIZE   16777223
#define POOL_SIZE   16

typedef struct pair_t pair_t;

enum node_state {FREE, SPLIT, ALLOC};

struct pair_t {
    int idx;
    enum node_state state;
    int size;
    pair_t* left;
    pair_t* right;
};

struct pair_t *pool_head;

int buddy_alloc(pair_t *p, int size);
int buddy_free(pair_t *p, int idx);
int buddy_get_size(pair_t *p, int idx);
void print_tree(pair_t *p);

#endif
