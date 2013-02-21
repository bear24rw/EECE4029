#include <stdio.h>
#include <stdlib.h>
#include "buddy.h"

int pool_free = POOL_SIZE;

int buddy_alloc(pair_t *p, int size, int level, int idx)
{
    int rt = 0;

    // if there is a pair below us, try them
    if (p->left != NULL && p->right != NULL) {
        printf("pair below, trying left (level %d size %d idx %d)\n", level, p->size, idx);
        rt = buddy_alloc(p->left, size, level+1, idx);
        if (rt < 0) {
            printf("pair below, trying right (level %d size %d idx %d)\n", level, p->size, idx);
            return buddy_alloc(p->right, size, level+1, idx+(p->size/2));
        } else {
            return rt;
        }
    }

    // if were not free return error
    if (!p->free) return -1;

    // we have more than enough space to make another pair
    if (p->size / 2 >= size) {
        printf("splitting (level %d size %d idx %d)\n", level, p->size, idx);
        // make a new pair
        p->left = (pair_t*)malloc(sizeof(pair_t));
        p->right = (pair_t*)malloc(sizeof(pair_t));
        p->left->free = 1;
        p->right->free = 1;
        p->left->size = p->size / 2;
        p->right->size = p->size / 2;
        p->left->idx = 0;
        p->right->idx = 0;
        p->left->left = NULL;
        p->right->right = NULL;

        return buddy_alloc(p->left, size, level+1, idx);

    // we don't have enough space for another pair but do we have
    // enough space to just allocate it here?
    } else if (p->size >= size) {
        // we're no longer free
        printf("allocating idx %d (level %d)\n", idx, level);
        p->free = 0;
        p->idx = idx;
        return idx;
    }

    return -1;
}

void print_tree(pair_t *p)
{
    // if there are children print them
    if (p->left != NULL) {
        print_tree(p->left);
        print_tree(p->right);
        return;
    }

    // otherwise print outself
    int i;
    for (i = 0; i < p->size; i++) {
        if (p->free)
            printf("-");
        else
            printf("%d,", p->idx);
    }
    printf("|");
}

