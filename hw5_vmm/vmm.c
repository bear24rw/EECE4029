/*
 * Copyright (C) 2013 Max Thrun
 *
 * EECE4029 - Introduction to Operating Systems
 * Assignment 5 - Virtual Memory Management
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "buddy.h"
#include "vmm.h"

int pool_size = DEFAULT_POOL_SIZE;
module_param(pool_size, int, 0);

int current_idx = 0;    /* currently selected page */
int read_size = 0;      /* how many bytes to read when read ioctl is called */

/* ioctl callback function */
long ioctl(struct file *file,
           unsigned int ioctl_num,
           unsigned long ioctl_param)
{
    char ch;    /* current character to or from userspace */
    int size;   /* size of page being written to */
    int bytes;  /* current number of bytes being read or written */

    switch (ioctl_num) {

        case IOCTL_ALLOC:

            printk(KERN_INFO "vmm: allocating %d bytes\n", (int)ioctl_param);
            return buddy_alloc((int)ioctl_param);

        case IOCTL_FREE:

            printk(KERN_INFO "vmm: freeing idx %d\n", (int)ioctl_param);
            return buddy_free((int)ioctl_param);

        case IOCTL_SET_IDX:

            current_idx = (int)ioctl_param;
            printk(KERN_INFO "vmm: setting idx to %d\n", current_idx);
            return 0;

        case IOCTL_SET_READ_SIZE:

            read_size = (int)ioctl_param;
            printk(KERN_INFO "vmm: setting read size to %d\n", read_size);
            return 0;

        case IOCTL_WRITE:

            /* reset the number of bytes written */
            bytes = 0;

            /* get the page size */
            size = buddy_size(current_idx);

            /* keep writing until some condition breaks us out */
            while (1) {

                /* get the next byte from userspace */
                get_user(ch, (char*)ioctl_param+bytes);

                /* if it's a null terminator we are finished writing */
                if (ch == '\0') break;

                /* if we are about to write outside the page error out */
                if (bytes > size) {
                    printk(KERN_INFO "vmm: writing out of allocated area\n");
                    return -1;
                }

                /* write the byte into the pool */
                *(buddy_pool+current_idx+bytes) = ch;
                printk(KERN_INFO "wrote %c to %d\n", ch, current_idx+bytes);

                /* go to the next byte */
                bytes++;
            }

            printk(KERN_INFO "vmm: wrote %d bytes\n", bytes);

            /* return how many bytes were written */
            return bytes;

        case IOCTL_READ:

            /* return error if trying to read more than the page size */
            if (read_size > buddy_size(current_idx)) {
                printk(KERN_INFO "vmm: read bigger than allocated area\n");
                return -1;
            }

            /* reset the number of bytes read */
            bytes = 0;

            /* keep reading bytes until we've satisfied the request */
            while (bytes < read_size) {

                /* get byte out of pool and send it to user */
                put_user(*(buddy_pool+current_idx+bytes), (char*)ioctl_param+bytes);

                /* go to the next byte */
                bytes++;
            }

            printk(KERN_INFO "vmm: read %d bytes\n", bytes);

            /* return how many bytes were read */
            return bytes;

        default: printk(KERN_INFO "vmm: unknown ioctl call\n");
    }

    return 0;
}

/* file operation callbacks */
struct file_operations file_ops = {
    .unlocked_ioctl = ioctl,
};

/* Module entry point */
static int __init init_mod(void)
{
    int ret;

    /* register ourselves as a character device */
    ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &file_ops);
    if (ret < 0) {
        printk(KERN_INFO "vmm: could not register the device (error: %d)\n", ret);
        return ret;
    }

    /* initialize the buddy pool and tree */
    ret = buddy_init(pool_size);
    if (ret < 0) {
        printk(KERN_INFO "vmm: could not allocate buddy pool\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "vmm: Module loaded successfully (device number: %d, pool size: %d)\n", MAJOR_NUM, pool_size);
    return 0;
}

/* Module exit point */
static void __exit exit_mod(void)
{
    buddy_kill();
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "vmm: Module unloaded successfully\n");
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Virtual Memory Manager");
