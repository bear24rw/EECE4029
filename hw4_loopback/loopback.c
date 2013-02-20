/*
 * Copyright (C) 2013 Max Thrun
 *
 * EECE4029 - Introduction to Operating Systems
 * Assignment 3 - Network Device Driver Loopback
 *
 * Writeup can be found here:
 * https://github.com/bear24rw/EECE4029/tree/master/hw4_loopback
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>

struct net_device *os0, *os1;

struct os_priv {
    struct net_device_stats stats;
    struct net_device *dev;
};

int os_open(struct net_device *dev) { netif_start_queue(dev); return 0; }
int os_stop(struct net_device *dev) { netif_stop_queue(dev); return 0; }

int os_create_header(struct sk_buff *skb, struct net_device *dev,
        unsigned short type, const void *daddr, const void *saddr,
        unsigned int len)
{

    struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

    /* convert byte order */
    eth->h_proto = htons(type);

    /* set the source address to our mac address */
    memcpy(eth->h_source, dev->dev_addr, dev->addr_len);

    /* set the destination address to the other mac address */
    memcpy(eth->h_dest, eth->h_source, dev->addr_len);
    eth->h_dest[ETH_ALEN-1] = (eth->h_dest[ETH_ALEN-1] == 5) ? 6 : 5;

    printk(KERN_INFO "loopback: created packet from %x:%x:%x:%x:%x:%x --> %x:%x:%x:%x:%x:%x\n",
            eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
            eth->h_dest[0],   eth->h_dest[1],   eth->h_dest[2],   eth->h_dest[3],   eth->h_dest[4],   eth->h_dest[5]);

    return dev->hard_header_len;
}

int os_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    int len;                    /* length of data in buffer */
    char *data;                 /* data is either directly from skb or the new padded buffer */
    char shortpkt[ETH_ZLEN];    /* if skb is too short we need copy it to new buffer and pad it */

    struct iphdr *ih;           /* ip header so we can flip the 3rd octet */
    u32 *saddr, *daddr;         /* source and dest in the ip header */

    struct net_device *dest;    /* destination device (opposite of xmit device) */
    struct sk_buff *new_skb;    /* packet we'll fill and say we recieved */

    /* pull data and length out of the socket buffer */
    data = skb->data;
    len = skb->len;

    /* if legnth is too short pad it with 0s */
    if (len < ETH_ZLEN) {
        memset(shortpkt, 0, ETH_ZLEN);
        memcpy(shortpkt, skb->data, skb->len);
        len = ETH_ZLEN;
        data = shortpkt;
    }

    /* save time stamp */
    dev->trans_start = jiffies;

    /* get the ip header */
    ih = (struct iphdr *)(data+sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;

    /* toggle the third octet to switch the network */
    ((u8 *)saddr)[2] ^= 1;
    ((u8 *)daddr)[2] ^= 1;

    printk(KERN_INFO "loopback: sending packet from %d.%d.%d.%d --> %d.%d.%d.%d\n",
            ((u8 *)saddr)[0], ((u8 *)saddr)[1], ((u8 *)saddr)[2], ((u8 *)saddr)[3],
            ((u8 *)daddr)[0], ((u8 *)daddr)[1], ((u8 *)daddr)[2], ((u8 *)daddr)[3]);

    /* rebuild the checksum */
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

    /* allocate a new buffer for the rx side */
    new_skb = dev_alloc_skb(len + 2);
    if (!new_skb) {
        printk(KERN_INFO "loopback: dropping packet\n");
        return 0;
    }

    /* copy the xmit data into it */
    skb_reserve(new_skb, 2);
    memcpy(skb_put(new_skb, len), data, len);

    /* destination is the other device */
    dest = (dev == os0) ? os1 : os0;

    /* set the device, protocol, and checksum flag */
    new_skb->dev = dest;
    new_skb->protocol = eth_type_trans(new_skb, dest);
    new_skb->ip_summed = CHECKSUM_UNNECESSARY;

    /* send it up the layers */
    netif_rx(new_skb);

    /* we handled this packet so free it */
    dev_kfree_skb(skb);

    return NETDEV_TX_OK;
}

struct net_device_stats *os_stats(struct net_device *dev) {
    struct os_priv *priv = netdev_priv(dev);
    return &priv->stats;
}

static const struct header_ops os_header_ops = {
    .create  = os_create_header,
};

static const struct net_device_ops os_device_ops = {
    .ndo_open = os_open,
    .ndo_stop = os_stop,
    .ndo_get_stats = os_stats,
    .ndo_start_xmit = os_start_xmit,
};

/*
 * Module entry point
 */
static int __init init_mod(void)
{
    int i;
    struct os_priv *priv0;
    struct os_priv *priv1;

    os0 = alloc_etherdev(sizeof(struct os_priv));
    os1 = alloc_etherdev(sizeof(struct os_priv));

    if (!os0 || !os1) {
        printk(KERN_INFO "loopback: error allocating net device\n");
        return -ENOMEM;
    }

    /* setup mac and broadcast addresses */
    for (i=0; i<6; i++) os0->dev_addr[i] = (unsigned char)i;
    for (i=0; i<6; i++) os1->dev_addr[i] = (unsigned char)i;
    for (i=0; i<6; i++) os0->broadcast[i] = (unsigned char)0xFF;
    for (i=0; i<6; i++) os1->broadcast[i] = (unsigned char)0xFF;

    /* set one of the devices to end in 6 so we have two different addresses */
    os1->dev_addr[5] = 0x6;

    /* set the names of the devices */
    memcpy(os0->name, "os0", 10);
    memcpy(os1->name, "os1", 10);

    /* set the callback functions */
    os0->netdev_ops = &os_device_ops;
    os0->header_ops = &os_header_ops;
    os1->netdev_ops = &os_device_ops;
    os1->header_ops = &os_header_ops;

    os0->hard_header_len = 14;
    os1->hard_header_len = 14;

    /* disable ARP since we won't response to it */
    os0->flags |= IFF_NOARP;
    os1->flags |= IFF_NOARP;

    /* get pointer to private area */
    priv0 = netdev_priv(os0);
    priv1 = netdev_priv(os1);

    /* clear the private area */
    memset(priv0, 0, sizeof(struct os_priv));
    memset(priv1, 0, sizeof(struct os_priv));

    /* set the device */
    priv0->dev = os0;
    priv1->dev = os1;

    if (register_netdev(os0) || register_netdev(os1))
        printk(KERN_INFO "loopback: error registering os0 or os1\n");
    else
        printk(KERN_INFO "loopback: os0 and os1 registered\n");

    printk(KERN_INFO "loopback: Module loaded successfully\n");
    return 0;
}

/*
 * Module exit point
 */
static void __exit exit_mod(void)
{
    struct os_priv *priv0;
    struct os_priv *priv1;

    if (os0) {
        priv0 = netdev_priv(os0);
        unregister_netdev(os0);
    }

    if (os1) {
        priv1 = netdev_priv(os1);
        unregister_netdev(os1);
    }

    printk(KERN_INFO "loopback: Module unloaded successfully\n");
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Network Loopback");
