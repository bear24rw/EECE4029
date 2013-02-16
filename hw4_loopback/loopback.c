/*
 * Copyright (C) 2013 Max Thrun
 *
 * EECE4029 - Introduction to Operating Systems
 * Assignment 3 - Network Device Driver Loopback
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

struct net_device *os0, *os1;

struct os_priv {
    struct net_device_stats stats;
    int status;
    struct os_packet *pkt;
    int rx_int_enabled;
    int tx_packetlen;
    u8 *tx_packetdata;
    struct sk_buff *skb;
    spinlock_t lock;
    struct net_device *dev;
};

struct os_packet {
    struct net_device *dev;
    int datalen;
    u8 data[ETH_DATA_LEN];
};

int os_create(struct sk_buff *skb, struct net_device *dev,
        unsigned short type, const void *daddr, const void *saddr,
        unsigned int len) {
    printk(KERN_INFO "loopback local create\n");
    return 0;
}

int local_rebuild(struct sk_buff *skb) {
    printk(KERN_INFO "local_rebuild:\n");
    return 0;
}


int os_open(struct net_device *dev) { printk(KERN_INFO "loopback open\n"); return 0;}
int os_stop(struct net_device *dev) { printk(KERN_INFO "loopback stop\n"); return 0;}
int os_start_xmit(struct sk_buff *skb, struct net_device *dev) { printk(KERN_INFO "loopback start xmit\n"); return 0; }

struct net_device_stats *os_stats(struct net_device *dev) {
    printk(KERN_INFO "loopback local get stats\n");
    return &dev->stats;
}


static const struct header_ops os_header_ops = {
    .create  = os_create,
};

static const struct net_device_ops os_device_ops = {
    .ndo_open = os_open,
    .ndo_stop = os_stop,
    .ndo_start_xmit = os_start_xmit,
    .ndo_get_stats = os_stats,
};


/*
 * Module entry point
 */
static int __init init_mod(void)
{
    int i;
    struct os_priv *priv;

    os0 = alloc_etherdev(sizeof(struct os_priv));
    if (!os0) {
        printk(KERN_INFO "loopback: error allocating net device\n");
        return -ENOMEM;
    }

    priv = netdev_priv(os0);

    for (i=0; i<6; i++) os0->dev_addr[i] = (unsigned char)i;
    for (i=0; i<6; i++) os0->broadcast[i] = (unsigned char)0xFF;

    os0->hard_header_len = 14;

    memcpy(os0->name, "os0", 10);

    os0->netdev_ops = &os_device_ops;
    os0->header_ops = &os_header_ops;
    os0->flags |= IFF_NOARP;

    memset(priv, 0, sizeof(struct os_priv));

    priv->dev = os0;
    priv->rx_int_enabled = 1;
    spin_lock_init(&priv->lock);
    priv->pkt = kmalloc(sizeof(struct os_packet), GFP_KERNEL);
    priv->pkt->dev = os0;

    if (register_netdev(os0))
        printk(KERN_INFO "error registering os0\n");
    else
        printk(KERN_INFO "os0 registered\n");

    printk(KERN_INFO "loopback: Module loaded successfully\n");
    return 0;
}

/*
 * Module exit point
 */
static void __exit exit_mod(void)
{
    struct os_priv *priv;

    if (os0) {
        priv = netdev_priv(os0);
        kfree(priv->pkt);
        unregister_netdev(os0);
    }


    printk(KERN_INFO "loopback: Module unloaded successfully\n");
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Network Loopback");
