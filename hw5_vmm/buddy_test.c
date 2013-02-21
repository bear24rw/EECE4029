#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "buddy.h"

void alloc_check(int bytes, int idx)
{
    int rt = buddy_alloc(buddy_head, bytes);
    assert(rt == idx);
    printf("ADDED %d BYTES\t|", bytes);
    print_tree(buddy_head);
    if (rt < 0) printf("  <-- returned %d", rt);
    printf("\n");
}

void free_check(int idx, int expected_rt)
{
    int rt = buddy_free(buddy_head, idx);
    assert(rt == expected_rt);
    printf("FREED IDX %d\t|", idx);
    print_tree(buddy_head);
    if (rt < 0) printf("  <-- returned %d", rt);
    printf("\n");
}

void size_check(int idx, int size)
{
    printf("SIZE IDX %d == %d\n", idx, size);
    int rt = buddy_get_size(buddy_head, idx);
    assert(rt == size);
}

void banner(const char *s)
{
    static int i = 1;
    printf("------------ Test %d: %s ------------\n", i++, s);
}

int main(void)
{
    banner("PDF");
    buddy_init(16);
    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    alloc_check(2, 8);
    free_check(4, 0);
    free_check(8, 0);
    free_check(0, 0);
    free_check(2, 0);

    banner("Fill");
    buddy_init(16);
    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    alloc_check(2, 8);
    alloc_check(4, 12);
    alloc_check(2, 10);
    free_check(4, 0);
    free_check(8, 0);
    free_check(0, 0);
    free_check(2, 0);

    banner("no space");
    buddy_init(16);
    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    alloc_check(2, 8);
    alloc_check(4, 12);
    alloc_check(2, 10);
    alloc_check(2, -1);

    banner("no split");
    buddy_init(16);
    alloc_check(16, 0);
    free_check(0, 0);

    banner("remove non existent");
    buddy_init(16);
    alloc_check(2, 0);
    alloc_check(4, 4);
    free_check(2, -1);
    free_check(8, -1);

    banner("two even");
    buddy_init(16);
    alloc_check(8, 0);
    alloc_check(8, 8);
    free_check(8, 0);
    free_check(0, 0);

    banner("first too big");
    buddy_init(16);
    alloc_check(20, -1);

    banner("free empty");
    buddy_init(16);
    free_check(0, -1);
    free_check(8, -1);

    banner("free out of range");
    buddy_init(16);
    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    free_check(20, -1);

    banner("pool size 1");
    buddy_init(1);
    alloc_check(2, -1);
    alloc_check(4, -1);
    alloc_check(1, 0);
    free_check(0, 0);

    banner("pool size 0");
    buddy_init(0);
    alloc_check(2, -1);
    alloc_check(4, -1);
    alloc_check(1, -1);
    free_check(0, -1);

    banner("test 1");
    buddy_init(16);
    alloc_check(8, 0);
    alloc_check(2, 8);
    alloc_check(4, 12);
    free_check(0, 0);
    alloc_check(2, 0);
    alloc_check(2, 2);
    alloc_check(2, 4);
    free_check(4, 0);
    alloc_check(6, -1);
    alloc_check(4, 4);
    free_check(0, 0);
    free_check(2, 0);
    free_check(4, 0);
    free_check(12, 0);
    free_check(8, 0);

    banner("test 2");
    buddy_init(8);
    alloc_check(8, 0);
    alloc_check(4, -1);
    free_check(2, -1);
    free_check(0, 0);
    alloc_check(4, 0);
    alloc_check(2, 4);
    alloc_check(4, -1);
    alloc_check(2, 6);
    free_check(4, 0);
    free_check(6, 0);
    free_check(0, 0);

    banner("size check PDF");
    buddy_init(16);
    alloc_check(2, 0);
    alloc_check(4, 4);
    alloc_check(2, 2);
    alloc_check(2, 8);
    size_check(0, 2);
    size_check(4, 4);
    size_check(2, 2);
    size_check(8, 2);
    free_check(4, 0);
    free_check(8, 0);
    free_check(0, 0);
    free_check(2, 0);

    banner("size check test 1");
    buddy_init(16);
    alloc_check(8, 0);
    alloc_check(2, 8);
    alloc_check(4, 12);
    free_check(0, 0);
    size_check(0, -1);
    size_check(8, 2);
    size_check(12, 4);
    alloc_check(2, 0);
    alloc_check(2, 2);
    alloc_check(2, 4);
    size_check(12, 4);
    size_check(0, 2);
    size_check(2, 2);
    size_check(4, 2);
    free_check(4, 0);
    size_check(4, -1);

    banner("size check pool size 1");
    buddy_init(1);
    alloc_check(1, 0);
    size_check(0, 1);
    free_check(0, 0);
    size_check(0, -1);

    banner("size check two even");
    buddy_init(16);
    alloc_check(8, 0);
    alloc_check(8, 8);
    size_check(0, 8);
    size_check(8, 8);
    free_check(8, 0);
    free_check(0, 0);
    size_check(0, -1);
    size_check(8, -1);


    return 0;
}

