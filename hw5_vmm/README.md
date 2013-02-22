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
void buddy_kill(void);
void buddy_print(void);
```
In order to compile for userspace you must define `NONKERNEL` when compiling so
that the proper header files and memory allocation functions will be used
(printk vs printf, kmalloc vs malloc, etc..).

The buddy allocator uses a binary tree where each node is in one of three
states: `FREE`, `SPLIT`, or `ALLOC`.  Additionally, each node keeps track of
its index into the memory pool `buddy_pool` as well as its size. The
`buddy_init` function allocates a new pool of size `size` and initializes the
head node of the tree.

When attempting to allocate space with the `buddy_alloc` function the tree is
traversed and free space is broken in half until the smallest space that will
properly allocate the request is achieved. If space cannot be found the
function returns -1 to indicate an error.

When freeing a page with `buddy_free` the tree is first traversed in all
directions to the bottom leaf nodes. It then checks if the leaf node is
associated with the page it is trying to free and if so marks it as `FREE` and
returns 0 to indicate it was successful.  When it is recursing back up the tree
the children of nodes marked as `SPLIT` are checked to see if both are `FREE`.
If both children are `FREE` they are deallocated and the parent node is marked
as `FREE` to coalesce the space.

The `buddy_size` function returns the number of bytes until the end of the
page.  For example if the page starts at index 0 and is 10 bytes long calling
`buddy_size(7)` will return 3. If the index does not fall in a page area it
returns -1.

The `buddy_kill` function traverses the whole tree and deallocates all nodes.

The `buddy_print` function draws out the pool for debugging purposes.

Kernel Module
-------------
The kernel module uses the buddy allocator to initialize a fixed pool of kernel
memory and then allocates and frees pages of this space using the buddy
allocator through ioctrl's.

The default pool size is set to (2^12)x(2^12) bytes, or 16,777,216 bytes. The
pool size can be adjusted at load time using the `pool_size` parameter. For
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

Kernel Module Test
------------------
In order to test the kernel module and exercise the ioctl's a user space
program `vmm_test` was developed that wraps the ioctl commands into simple
functions:

```C
int get_mem(int fd, int bytes)
int free_mem(int fd, int idx)
int write_mem(int fd, int idx, char *buf)
int read_mem(int fd, int idx, char *buf, int size)
```

With these functions in place we can write a simple test program to exercise our module:

```C
int mem, ref;
char buffer[4096];

mem = open("/dev/vmm", 0);
if (mem < 0) {
    printf("Can't open device file\n");
    exit(-1);
}

ref = get_mem(mem, 100);
sprintf(buffer, "Hello buddy");
write_mem(mem, ref, buffer);
read_mem(mem, ref+3, buffer, 10);
printf("buffer: %s\n", buffer);
free_mem(mem, ref);
```

We can compile the test program with:

```
make vmm_test
```

Running it gives us our expected result:

```
% ./vmm_test
buffer: lo buddy
```

Looking at `dmesg` we can also see some messages from our module that match our
userspace result:

```
Feb 21 21:37:52 gentoo-vm kernel: vmm: allocating 100 bytes
Feb 21 21:37:52 gentoo-vm kernel: vmm: setting idx to 0
Feb 21 21:37:52 gentoo-vm kernel: wrote H to 0
Feb 21 21:37:52 gentoo-vm kernel: wrote e to 1
Feb 21 21:37:52 gentoo-vm kernel: wrote l to 2
Feb 21 21:37:52 gentoo-vm kernel: wrote l to 3
Feb 21 21:37:52 gentoo-vm kernel: wrote o to 4
Feb 21 21:37:52 gentoo-vm kernel: wrote   to 5
Feb 21 21:37:52 gentoo-vm kernel: wrote b to 6
Feb 21 21:37:52 gentoo-vm kernel: wrote u to 7
Feb 21 21:37:52 gentoo-vm kernel: wrote d to 8
Feb 21 21:37:52 gentoo-vm kernel: wrote d to 9
Feb 21 21:37:52 gentoo-vm kernel: wrote y to 10
Feb 21 21:37:52 gentoo-vm kernel: vmm: wrote 11 bytes
Feb 21 21:37:52 gentoo-vm kernel: vmm: setting idx to 3
Feb 21 21:37:52 gentoo-vm kernel: vmm: setting read size to 10
Feb 21 21:37:52 gentoo-vm kernel: vmm: read 10 bytes
Feb 21 21:37:52 gentoo-vm kernel: vmm: freeing idx 0
```

Buddy Allocator Unit Tests
--------------------------
In order to test the buddy allocator and exercise various use cases a user
space program called `buddy_test` was developed that allows easy testing of
multiple scenarios.  For instance to allocate a pool of 16 bytes and then
allocate two pages one being 2 bytes and the other being 4 bytes we can write:

```C
banner("Example");
buddy_init(16);
alloc_check(2, 0);
alloc_check(4, 4);
buddy_kill();
```

The first argument of `alloc_check` tells how many bytes we want to
allocate. The second argument is the expected index of the page. If the buddy
allocator returns a different index then the one we were expecting the
`assert` statement in `alloc_check` will fail to indicate the error.

After adding our test to `buddy_test.c` we can compile it with:

```
make buddy_test
```

And run it:

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

The `buddy_test` program exercises 19 different situations that were
determined to be valuable tests during development. Since the buddy allocator
is dealing with allocating and freeing memory as it builds the tree it is
highly beneficial to be able to test it like this in user space so we don't
have to deal with possible kernel panics.
