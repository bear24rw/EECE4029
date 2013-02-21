#ifndef _BUDDY_H_
#define _BUDDY_H_

/*
 * We need to use different headers and functions
 * depending of if we are compiling as a kernel
 * module or not.
 */
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

/* Node can only be in one of 3 states */
enum node_state {FREE, SPLIT, ALLOC};

typedef struct node_t node_t;

struct node_t {
    int idx;                /* index into the pool */
    enum node_state state;  /* state of the node */
    int size;               /* how many bytes in the pool */
    node_t* left;           /* if split we make two buddies below us */
    node_t* right;
};

/* Pool is global so it can be read and written */
extern char *buddy_pool;

int buddy_init(int size);
int buddy_alloc(int size);
int buddy_free(int idx);
int buddy_size(int idx);
void buddy_print(void);
void buddy_kill(void);

#endif
