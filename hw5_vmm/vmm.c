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
#include "vmm.h"

int current_idx = 0;
int read_size = 0;

long ioctl(struct file *file,
           unsigned int ioctl_num,
           unsigned long ioctl_param)
{
    char ch;
    int size;
    int page;
    int bytes;

    switch (ioctl_num) {
        case IOCTL_ALLOC:
            size = (int)ioctl_param;
            printk(KERN_INFO "vmm: allocating %d bytes\n", size);
            return 0xbeef;
            break;

        case IOCTL_FREE:
            page = (int)ioctl_param;
            printk(KERN_INFO "vmm: freeing idx %d\n", page);
            break;

        case IOCTL_SET_IDX:
            current_idx = (int)ioctl_param;
            printk(KERN_INFO "vmm: setting idx to %d\n", current_idx);
            break;

        case IOCTL_SET_READ_SIZE:
            read_size = (int)ioctl_param;
            printk(KERN_INFO "vmm: setting read size to %d\n", read_size);
            break;

        case IOCTL_WRITE:
            bytes = 0;
            while (1) {
                get_user(ch, (char*)ioctl_param+bytes);
                if (ch == '\0') {
                    printk(KERN_INFO "vmm: wrote 0 bytes\n");
                    break;
                }
                printk(KERN_INFO "writing %c to %d\n", ch, current_idx+bytes);
                *(pool+current_idx+bytes) = ch;
                bytes++;
            }
            printk(KERN_INFO "vmm: wrote %d bytes\n", bytes);
            return bytes;

        case IOCTL_READ:
            bytes = 0;
            while (bytes < read_size) {
                put_user(*(pool+current_idx+bytes), (char*)ioctl_param+bytes);
                bytes++;
            }
            printk(KERN_INFO "vmm: read %d bytes\n", bytes);
            return bytes;
    }

    return 0;
}

static int open(struct inode *inode, struct file *file) {printk(KERN_INFO "vmm: open\n"); return 0;}
static int release(struct inode *inode, struct file *file) {printk(KERN_INFO "vmm: release\n"); return 0;}
static ssize_t read(struct file *file,
        char __user * buffer,/* buffer to be filled with data */
        size_t length,/* length of the buffer     */
        loff_t * offset)
{
    printk(KERN_INFO "vmm: read\n");
    return 0;
}
static ssize_t write(struct file *file,
        const char __user *buffer,
        size_t length,
        loff_t *offset)
{
    printk(KERN_INFO "vmm: write\n");
    return 0;
}


struct file_operations file_ops = {
    .read = read,
    .write = write,
    .unlocked_ioctl = ioctl,
    .open = open,
    .release = release
};

/*
 * Module entry point
 */
static int __init init_mod(void)
{
    int ret;
    ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &file_ops);
    if (ret < 0) {
        printk(KERN_INFO "vmm: could not register the device (error: %d)\n", ret);
        return ret;
    }

    pool = vmalloc(POOL_SIZE);
    if (!pool) {
        printk(KERN_INFO "vmm: could not allocate pool\n");
        return -ENOMEM;
    }

    pool_free = POOL_SIZE;

    pool_head = (pair_t*)kmalloc(sizeof(pair_t));
    pool_head.left = NULL;
    pool_head.right = NULL;
    pool_head.free = 1;
    pool_head.size = POOL_SIZE;
    pool_head.idx = 0;

    printk(KERN_INFO "vmm: Module loaded successfully (device number: %d)\n", MAJOR_NUM);
    return 0;
}

/*
 * Module exit point
 */
static void __exit exit_mod(void)
{
    vfree(pool);
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "vmm: Module unloaded successfully\n");
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Virtual Memory Manager");
