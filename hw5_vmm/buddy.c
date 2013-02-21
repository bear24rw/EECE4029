#include <stdio.h>
#include <stdlib.h>
#include "buddy.h"

int buddy_alloc(pair_t *p, int size)
{
    int rt = -1;

    // if there is a pair below us, try them
    if (p->state == SPLIT) {

        rt = buddy_alloc(p->left, size);
        if (rt >= 0) return rt;

        return buddy_alloc(p->right, size);
    }

    // if were not free return error
    if (p->state != FREE) return -1;

    // if were too small return error
    if (p->size < size) return -1;

    // we have enough space to make another pair
    if (p->size / 2 >= size) {
        //printf("splitting (size %d idx %d)\n", p->size, p->idx);
        p->state = SPLIT;

        // make a new pair
        p->left = (pair_t*)bmalloc(sizeof(pair_t));
        p->right = (pair_t*)bmalloc(sizeof(pair_t));
        p->left->state = FREE;
        p->right->state = FREE;
        p->left->size = p->size / 2;
        p->right->size = p->size / 2;
        p->left->idx = p->idx;
        p->right->idx = p->idx + (p->size / 2);
        p->left->left = NULL;
        p->right->right = NULL;

        return buddy_alloc(p->left, size);

    }

    // just allocate it here
    // we're no longer free
    //printf("allocating idx %d \n", p->idx);
    p->state = ALLOC;
    return p->idx;

}

int buddy_free(pair_t *p, int idx)
{
    int rt_left = -1;
    int rt_right = -1;

    if (p->state == SPLIT) {

        // recurse down each path
        rt_left = buddy_free(p->left, idx);
        rt_right = buddy_free(p->right, idx);

        // check if we should merge this split
        if (p->left->state == FREE && p->right->state == FREE) {
            //printf("merging idx %d\n", idx);
            free(p->left);
            free(p->right);
            p->left = NULL;
            p->right = NULL;
            p->state = FREE;
        }

        // if either path freed the slot return success
        if (rt_left == 0 || rt_right == 0) return 0;

        return -1;
    }

    if (p->idx == idx && p->state == ALLOC) {
        p->state = FREE;
        return 0;
    }

    return -1;
}

int buddy_get_size(pair_t *p, int idx)
{
    int rt = -1;

    if (p->state == SPLIT) {
        rt = buddy_get_size(p->left, idx);
        if (rt < 0)
            return buddy_get_size(p->right, idx);
        else
            return rt;
    }

    if (p->idx == idx && p->state == ALLOC)
        return p->size;

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
        if (p->state == FREE)
            printf("-,");
        else
            printf("%d,", p->idx);
    }
    printf("|");
}

