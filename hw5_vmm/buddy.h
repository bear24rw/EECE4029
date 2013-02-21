#ifndef _BUDDY_H_
#define _BUDDY_H_

//#define POOL_SIZE   16777223
#define POOL_SIZE   16*3

typedef struct pair_t pair_t;

#define FREE 0
#define SPLIT 1
#define ALLOC 2

struct pair_t {
    int idx;
    int state;
    int size;
    pair_t* left;
    pair_t* right;
};

struct pair_t *pool_head;

int buddy_alloc(pair_t *p, int size, int level);
int buddy_free(pair_t *p, int idx, int level);
void print_tree(pair_t *p);

#endif
