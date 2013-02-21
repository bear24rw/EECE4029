#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "buddy.h"

char *pool;

void alloc_check(int bytes, int idx)
{
    assert(buddy_alloc(pool_head, bytes, 0, 0) == idx);
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
    pool_head->free = 1;
    pool_head->size = POOL_SIZE;
    pool_head->idx = 0;

    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    alloc_check(2, 8);
    alloc_check(4, 12);
    alloc_check(4, -1);
    alloc_check(2, 10);

    return 0;
}

