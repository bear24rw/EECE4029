#include <stdio.h>
#include <stdlib.h>
#include "buddy.h"

char *buddy_pool;
struct node_t *buddy_head;

int buddy_init(int size)
{
    if (buddy_pool) free(buddy_pool);
    if (buddy_head) free(buddy_head);

    buddy_pool = bmalloc(size);
    if (!buddy_pool) {
        printb("vmm: could not allocate pool\n");
        return -1;
    }

    buddy_head = (node_t*)bmalloc(sizeof(node_t));
    if (!buddy_head) {
        printb("vmm: could not allocate buddy head\n");
        return -1;
    }

    buddy_head->left = NULL;
    buddy_head->right = NULL;
    buddy_head->state = FREE;
    buddy_head->size = size;
    buddy_head->idx = 0;

    return 0;
}

int buddy_alloc(node_t *n, int size)
{
    int rt = -1;

    // if there is a pair below us, try them
    if (n->state == SPLIT) {

        rt = buddy_alloc(n->left, size);
        if (rt >= 0) return rt;

        return buddy_alloc(n->right, size);
    }

    // if were not free return error
    if (n->state != FREE) return -1;

    // if were too small return error
    if (n->size < size) return -1;

    // we have enough space to make another pair
    if (n->size / 2 >= size) {
        //printf("splitting (size %d idx %d)\n", n->size, n->idx);
        n->state = SPLIT;

        // make a new pair
        n->left = (node_t*)bmalloc(sizeof(node_t));
        n->right = (node_t*)bmalloc(sizeof(node_t));
        n->left->state = FREE;
        n->right->state = FREE;
        n->left->size = n->size / 2;
        n->right->size = n->size / 2;
        n->left->idx = n->idx;
        n->right->idx = n->idx + (n->size / 2);
        n->left->left = NULL;
        n->right->right = NULL;

        return buddy_alloc(n->left, size);

    }

    // just allocate it here
    // we're no longer free
    //printf("allocating idx %d \n", n->idx);
    n->state = ALLOC;
    return n->idx;

}

int buddy_free(node_t *n, int idx)
{
    int rt_left = -1;
    int rt_right = -1;

    if (n->state == SPLIT) {

        // recurse down each path
        rt_left = buddy_free(n->left, idx);
        rt_right = buddy_free(n->right, idx);

        // check if we should merge this split
        if (n->left->state == FREE && n->right->state == FREE) {
            //printf("merging idx %d\n", idx);
            free(n->left);
            free(n->right);
            n->left = NULL;
            n->right = NULL;
            n->state = FREE;
        }

        // if either path freed the slot return success
        if (rt_left == 0 || rt_right == 0) return 0;

        return -1;
    }

    if (n->idx == idx && n->state == ALLOC) {
        n->state = FREE;
        return 0;
    }

    return -1;
}

int buddy_get_size(node_t *n, int idx)
{
    int rt = -1;

    if (n->state == SPLIT) {
        rt = buddy_get_size(n->left, idx);
        if (rt < 0)
            return buddy_get_size(n->right, idx);
        else
            return rt;
    }

    if (n->idx == idx && n->state == ALLOC)
        return n->size;

    return -1;
}

void print_tree(node_t *n)
{
    // if there are children print them
    if (n->left != NULL) {
        print_tree(n->left);
        print_tree(n->right);
        return;
    }

    // otherwise print outself
    int i;
    for (i = 0; i < n->size; i++) {
        if (n->state == FREE)
            printf("-,");
        else
            printf("%d,", n->idx);
    }
    printf("|");
}

