#include <stdio.h>
#include <stdlib.h>
#include "buddy.h"

int pool_free = POOL_SIZE;

int buddy_alloc(pair_t *p, int size, int level)
{
    int rt = -1;

    // if there is a pair below us, try them
    if (p->state == SPLIT) {
        //printf("pair below, trying left (level %d size %d idx %d)\n", level, p->size, p->idx);
        rt = buddy_alloc(p->left, size, level+1);
        if (rt < 0) {
            //printf("pair below, trying right (level %d size %d idx %d)\n", level, p->size, p->idx);
            return buddy_alloc(p->right, size, level+1);
        } else {
            return rt;
        }
    }

    // if were not free return error
    if (p->state != FREE) return -1;

    // we have more than enough space to make another pair
    if (p->size / 2 >= size) {
        //printf("splitting (level %d size %d idx %d)\n", level, p->size, p->idx);
        p->state = SPLIT;

        // make a new pair
        p->left = (pair_t*)malloc(sizeof(pair_t));
        p->right = (pair_t*)malloc(sizeof(pair_t));
        p->left->state = FREE;
        p->right->state = FREE;
        p->left->size = p->size / 2;
        p->right->size = p->size / 2;
        p->left->idx = p->idx;
        p->right->idx = p->idx + (p->size / 2);
        p->left->left = NULL;
        p->right->right = NULL;

        return buddy_alloc(p->left, size, level+1);

    // we don't have enough space for another pair but do we have
    // enough space to just allocate it here?
    } else if (p->size >= size) {
        // we're no longer free
        //printf("allocating idx %d (level %d)\n", p->idx, level);
        p->state = ALLOC;
        return p->idx;
    }

    return -1;
}

int buddy_free(pair_t *p, int idx, int level)
{
    int rt = -1;

    // we are end node
    if (p->left == NULL) return -1;

    // we are the parent to two leaf nodes

    //printf("checking left idx %d\n", p->left->idx);
    if (p->left->idx == idx && p->left->state == ALLOC) {
        p->left->state = FREE;
        //printf("freeing idx %d\n", idx);
        rt = 0;
    }

    //printf("checking right idx %d\n", p->right->idx);
    if (p->right->idx == idx && p->right->state == ALLOC) {
        p->right->state = FREE;
        //printf("freeing idx %d\n", idx);
        rt = 0;
    }

    if (p->left->state == FREE && p->right->state == FREE) {
        //printf("merging idx %d\n", idx);
        free(p->left);
        free(p->right);
        p->left = NULL;
        p->right = NULL;
        p->state = FREE;
        return 0;
    }

    int rt2 = -1;
    if (rt < 0) {
        rt2 = buddy_free(p->left, idx, level+1);
        if (rt2 < 0) {
            return buddy_free(p->right, idx, level+1);
        } else {
            return rt2;
        }
    }

    return rt;
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

