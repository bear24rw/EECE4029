Virtual Memory Manager
======================

This module implements a virtual memory manager that uses a buddy allocator.
The buddy allocator is implemented as a seperate standalone library and can be
utilized in user-space as well.

Buddy Allocator
---------------
The buddy allocator is self contained in `buddy.c` and `buddy.h` and exposes
the following functions:

```C
int buddy_init(int size);
int buddy_alloc(int size);
int buddy_free(int idx);
int buddy_size(int idx);
void buddy_print(void);
void buddy_kill(void);
```
In order to compile for userspace you must define `NONKERNEL` when compiling so that
the proper header files and memory allocation functions will be used.

The buddy allocator uses a binary tree where each node is in one of three states:
`FREE`, `SPLIT`, or `ALLOC`.  Additionally, each node keeps track of its index
into the memory pool `buddy\_pool` as well as its size.  When allocating space
the tree is traversed and free space is broken in half until the smallest space
that will properly allocate the request is achieved. If space cannot be found
the function returns -1 to indicate an error. When freeing space the tree is
first traversed in all directions to the bottom leaf nodes. It then checks if
the leaf node is the one it is trying to free and if so marks it as `FREE` and
returns 0 to indicate it was successful. When it is recursing back up the tree
the children of nodes marked as `SPLIT` are checked to see if both are `FREE`.
If both children are `FREE` they are deallocated and the parent node is marked
as `FREE` to coalesce the space.

Kernel Module
-------------
The virtual memory kernel module allocates a fixed pool of space in the kernel.
It then allocates pages of this space based on the buddy allocator described
above.

The default pool size is set to (2^12)*(2^12) bytes, or 16,777,216 bytes. The
pool size can be adjusted at load time using the `pool\_size` parameter. For
instance, to allocate a pool of 512 bytes:

```
sudo insmod vmm_dev.ko pool_size=512
```

The  module presents itself as a character device with major number `100`.
After loading the module the device file can be created with the `mknod`
command:

```
sudo mknod /dev/vmm c 100 0
```

The memory manger can then be interfaced through the following ioctrl's in userspace:

```
IOCTL_ALLOC
IOCTL_FREE
IOCTL_SET_IDX
IOCTL_SET_READ_SIZE
IOCTL_WRITE
IOCTL_READ
```

Buddy Allocator Unit Tests
--------------------------
In order to test the buddy allocator and exercise edge conditions a user space
`buddy\_test` program was developed that allows easy testing of multiple
scenarios.  For instance to allocate a pool of 16 bytes and then allocate two
pages one being 2 bytes and the other being 4 bytes we can write:

```C
banner("Example");
buddy_init(16);
alloc_check(2, 0);
alloc_check(4, 4);
buddy_kill();
```

The first argument of `alloc\_check()` tells how many bytes we want to
allocate. The second argument is the expected index of the page. If buddy
allocator returns a different index then the one we were expected the
`assert()` statement in `alloc\_check()` will fail to indicate the error.

After adding our test to `buddy\_test.c` we can compile it with:

```
make buddy_test
```

And running it:

```
% ./buddy_test
------------ Test 1: Example ------------
ALLOCED 2 BYTES |0,0,|-,-,|-,-,-,-,|-,-,-,-,-,-,-,-,|
ALLOCED 4 BYTES |0,0,|-,-,|4,4,4,4,|-,-,-,-,-,-,-,-,|
```

The test prints out a drawing of the pool after each step. We can
see that our 2 byte allocation ended up at index 0 as expected and
our 4 byte allocation ended up at index 4 like expected.

Using this method it is easy to extend the tests to check for any
corner cases that might arise such as trying to free pages that
haven't been allocated:

```C
banner("remove non existent");
buddy_init(16);
alloc_check(2, 0);
alloc_check(4, 4);
free_check(2, -1);
free_check(8, -1);
buddy_kill();
```
```
------------ Test 6: remove non existent ------------
ALLOCED 2 BYTES |0,0,|-,-,|-,-,-,-,|-,-,-,-,-,-,-,-,|
ALLOCED 4 BYTES |0,0,|-,-,|4,4,4,4,|-,-,-,-,-,-,-,-,|
FREED IDX 2 |0,0,|-,-,|4,4,4,4,|-,-,-,-,-,-,-,-,|  <-- returned -1
FREED IDX 8 |0,0,|-,-,|4,4,4,4,|-,-,-,-,-,-,-,-,|  <-- returned -1
```

Or allocating space that is not a power of 2:

```C
banner("non power 2");
buddy_init(16);
alloc_check(3, 0);
alloc_check(6, 8);
alloc_check(1, 4);
alloc_check(1, 5);
```
```
------------ Test 19: non power 2 ------------
ALLOCED 3 BYTES |0,0,0,0,|-,-,-,-,|-,-,-,-,-,-,-,-,|
ALLOCED 6 BYTES |0,0,0,0,|-,-,-,-,|8,8,8,8,8,8,8,8,|
ALLOCED 1 BYTES |0,0,0,0,|4,|-,|-,-,|8,8,8,8,8,8,8,8,|
ALLOCED 1 BYTES |0,0,0,0,|4,|5,|-,-,|8,8,8,8,8,8,8,8,|
```

`buddy\_test.c` tests 19 situations that were determined to be valuable tests
during development. Since the buddy allocator is dealing with allocating and freeing
memory as it builds the tree it is highly beneficial to be able to test it like this
in user space.
