#ifndef _BUDDY_H_
#define _BUDDY_H_

#ifdef NONKERNEL
#include <stdio.h>
#include <stdlib.h>
#define bmalloc(...) malloc(__VA_ARGS__)
#define bfree(...) free(__VA_ARGS__)
#define printb(...) printf(__VA_ARGS__)
#else
#include <linux/vmalloc.h>
#define bmalloc(...) vmalloc(__VA_ARGS__)
#define bfree(...) vfree(__VA_ARGS__)
#define printb(...) printk(KERN_INFO __VA_ARGS__)
#endif

typedef struct node_t node_t;

enum node_state {FREE, SPLIT, ALLOC};

struct node_t {
    int idx;
    enum node_state state;
    int size;
    node_t* left;
    node_t* right;
};

extern char *buddy_pool;

int buddy_init(int size);
int buddy_alloc(int size);
int buddy_free(int idx);
int buddy_size(int idx);
void buddy_print(void);
void buddy_kill(void);

#endif
