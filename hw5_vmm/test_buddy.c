#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "buddy.h"

char *pool;

void alloc_check(int bytes, int idx)
{
    int rt = buddy_alloc(pool_head, bytes, 0);
    assert(rt == idx);
    printf("ADDED %d BYTES\t", bytes);
    printf("|");
    print_tree(pool_head);
    printf("\n");
}

void free_check(int idx, int rt)
{
    int ret = buddy_free(pool_head, idx, 0);
    assert(ret == rt);
    printf("FREED IDX %d\t", idx);
    printf("|");
    print_tree(pool_head);
    printf("\n");
}

int main(void)
{
    pool = malloc(POOL_SIZE);
    if (!pool) {
        printf("vmm: could not allocate pool\n");
        return 1;
    }

    pool_head = (pair_t*)malloc(sizeof(pair_t));
    pool_head->left = NULL;
    pool_head->right = NULL;
    pool_head->state = FREE;
    pool_head->size = POOL_SIZE;
    pool_head->idx = 0;

    // num_bytes, resulting_idx
    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    alloc_check(2, 8);

    free_check(4, 0);
    free_check(8, 0);
    free_check(0, 0);
    free_check(2, 0);

    return 0;
}

