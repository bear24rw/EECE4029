#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "vmm.h"

int get_mem(int fd, int bytes)
{
    return ioctl(fd, IOCTL_ALLOC, bytes);
}

int free_mem(int fd, int idx)
{
    return ioctl(fd, IOCTL_FREE, idx);
}

int write_mem(int fd, int idx, char *buf)
{
    int rt = 0;

    /* set the page we want to write to */
    rt = ioctl(fd, IOCTL_SET_IDX, idx);
    if (rt < 0) return rt;

    /* execute the write */
    rt = ioctl(fd, IOCTL_WRITE, buf);

    return rt;
}

int read_mem(int fd, int idx, char *buf, int size)
{
    int rt = 0;

    /* set the page we want to read from */
    rt = ioctl(fd, IOCTL_SET_IDX, idx);
    if (rt < 0) return rt;

    /* set how many bytes we want to read */
    rt = ioctl(fd, IOCTL_SET_READ_SIZE, size);
    if (rt < 0) return rt;

    /* execute the read */
    rt = ioctl(fd, IOCTL_READ, buf);

    return rt;
}


int main(void)
{
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

    return 0;
}

