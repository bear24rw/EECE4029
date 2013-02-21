#ifndef _BUDDY_H_
#define _BUDDY_H_

#ifdef NONKERNEL
#define bmalloc(...) malloc(__VA_ARGS__)
#define printb(...) printf(__VA_ARGS__)
#else
#define bmalloc(...) vmalloc(__VA_ARGS__)
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
extern struct node_t *buddy_head;

int buddy_init(int size);
int buddy_alloc(node_t *n, int size);
int buddy_free(node_t *n, int idx);
int buddy_get_size(node_t *n, int idx);
void print_tree(node_t *n);

#endif
