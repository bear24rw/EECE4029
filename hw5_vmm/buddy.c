#include "buddy.h"

char *buddy_pool;
struct node_t *buddy_head;

int buddy_init(int size)
{
    if (buddy_pool) bfree(buddy_pool);
    if (buddy_head) bfree(buddy_head);

    buddy_pool = bmalloc(size);
    if (!buddy_pool) {
        return -1;
    }

    buddy_head = (node_t*)bmalloc(sizeof(node_t));
    if (!buddy_head) {
        return -1;
    }

    buddy_head->left = NULL;
    buddy_head->right = NULL;
    buddy_head->state = FREE;
    buddy_head->size = size;
    buddy_head->idx = 0;

    return 0;
}

void buddy_kill(void)
{
    /* TODO walk tree and free all nodes */
}

int _buddy_alloc(node_t *n, int size)
{
    int rt = -1;

    // if there is a pair below us, try them
    if (n->state == SPLIT) {

        rt = _buddy_alloc(n->left, size);
        if (rt >= 0) return rt;

        return _buddy_alloc(n->right, size);
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

        return _buddy_alloc(n->left, size);

    }

    // just allocate it here
    // we're no longer free
    //printf("allocating idx %d \n", n->idx);
    n->state = ALLOC;
    return n->idx;

}

int _buddy_free(node_t *n, int idx)
{
    int rt_left = -1;
    int rt_right = -1;

    if (n->state == SPLIT) {

        // recurse down each path
        rt_left = _buddy_free(n->left, idx);
        rt_right = _buddy_free(n->right, idx);

        // check if we should merge this split
        if (n->left->state == FREE && n->right->state == FREE) {
            //printf("merging idx %d\n", idx);
            bfree(n->left);
            bfree(n->right);
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

int _buddy_size(node_t *n, int idx)
{
    int rt = -1;

    if (n->state == SPLIT) {
        rt = _buddy_size(n->left, idx);
        if (rt < 0)
            return _buddy_size(n->right, idx);
        else
            return rt;
    }

    if (n->idx == idx && n->state == ALLOC)
        return n->size;

    return -1;
}

void _buddy_print(node_t *n)
{
    int i;

    // if there are children print them
    if (n->left != NULL) {
        _buddy_print(n->left);
        _buddy_print(n->right);
        return;
    }

    // otherwise print outself
    for (i = 0; i < n->size; i++) {
        if (n->state == FREE)
            printb("-,");
        else
            printb("%d,", n->idx);
    }
    printb("|");
}

/* recursion wrappers */
int buddy_alloc(int size) { return _buddy_alloc(buddy_head, size); }
int buddy_free(int idx) { return _buddy_free(buddy_head, idx); }
int buddy_size(int idx) { return _buddy_size(buddy_head, idx); }
void buddy_print(void) { _buddy_print(buddy_head); }
