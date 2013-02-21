#ifndef _BUDDY_H_
#define _BUDDY_H_

//#define POOL_SIZE   16777223
#define POOL_SIZE   16

typedef struct pair_t pair_t;

struct pair_t {
    int idx;
    char free;
    int size;
    pair_t* left;
    pair_t* right;
};

struct pair_t *pool_head;

int buddy_alloc(pair_t *p, int size, int level, int idx);
void print_tree(pair_t *p);

#endif
