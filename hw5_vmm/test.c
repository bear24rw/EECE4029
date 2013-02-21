#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "vmm.h"

int vmm_alloc(int fd, int bytes)
{
    int idx = 0;

    idx = ioctl(fd, IOCTL_ALLOC, bytes);
    if (idx < 0) {
        printf("vmm_alloc failed: %d\n", idx);
        exit(-1);
    }

    return idx;
}

void vmm_free(int fd, int idx)
{
    int rt = ioctl(fd, IOCTL_FREE, idx);
    if (rt < 0) {
        printf("vmm_free failed: %d\n", rt);
        exit(-1);
    }
}

int vmm_write(int fd, int idx, char *buf)
{
    int rt = 0;

    rt = ioctl(fd, IOCTL_SET_IDX, idx);
    if (rt < 0) {
        printf("vmm_set_idx %d failed\n", idx);
        exit(-1);
    }

    rt = ioctl(fd, IOCTL_WRITE, buf);

    return rt;
}

int vmm_read(int fd, int idx, char *buf, int size)
{
    int rt = 0;

    rt = ioctl(fd, IOCTL_SET_IDX, idx);
    if (rt < 0) {
        printf("vmm_set_idx %d failed\n", idx);
        exit(-1);
    }

    rt = ioctl(fd, IOCTL_SET_READ_SIZE, size);
    if (rt < 0) {
        printf("vmm_set_read_size %d failed\n", size);
        exit(-1);
    }

    rt = ioctl(fd, IOCTL_READ, buf);

    return rt;
}


int main(void)
{
    int fd;

    fd = open("/dev/vmm", 0);
    if (fd < 0) {
        printf("Can't open device file\n");
        exit(-1);
    }

    char buf[4] = "max\0";
    char out[4];
    out[3] = '\0';

    printf("Allocated idx %d\n", vmm_alloc(fd, 2));
    printf("Allocated idx %d\n", vmm_alloc(fd, 4));
    printf("Allocated idx %d\n", vmm_alloc(fd, 2));
    /*
    printf("Writing to idx 5: %d\n", vmm_write(fd, 0, buf));
    printf("Reading from idx 5: %d\n", vmm_read(fd, 0, out, 4));
    printf("Read: %s\n", out);
    printf("Freeing\n"); vmm_free(fd, 0);
    */

    return 0;
}

