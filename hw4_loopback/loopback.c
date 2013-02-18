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


int os_create_header(struct sk_buff *skb, struct net_device *dev,
        unsigned short type, const void *daddr, const void *saddr,
        unsigned int len) {

    struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

    eth->h_proto = htons(type);

    memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
    memcpy(eth->h_dest, eth->h_source, dev->addr_len);

    eth->h_dest[ETH_ALEN-1] = (eth->h_dest[ETH_ALEN-1] == 5) ? 6 : 5;

    printk("created packet from %x:%x:%x:%x:%x:%x to %x:%x:%x:%x:%x:%x\n",
            eth->h_source[0],
            eth->h_source[1],
            eth->h_source[2],
            eth->h_source[3],
            eth->h_source[4],
            eth->h_source[5],
            eth->h_dest[0],
            eth->h_dest[1],
            eth->h_dest[2],
            eth->h_dest[3],
            eth->h_dest[4],
            eth->h_dest[5]
          );

    return dev->hard_header_len;
}


int os_open(struct net_device *dev) { netif_start_queue(dev); return 0;}
int os_stop(struct net_device *dev) { netif_stop_queue(dev); return 0;}

int os_start_xmit(struct sk_buff *skb, struct net_device *dev) {

    int len;
    char *data, shortpkt[ETH_ZLEN];

    struct iphdr *ih;
    struct net_device *dest;
    u32 *saddr, *daddr;

    struct sk_buff *new_skb;

    printk(KERN_INFO "loopback start xmit\n");

    data = skb->data;
    len = skb->len;
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

    /* rebuild the checksum */
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);


    /* destination is the other device */
    dest = (dev == os0) ? os1 : os0;

    /* allocate room for new packet */
    new_skb = dev_alloc_skb(len + 2);
    if (!skb) {
        printk(KERN_INFO "os_rx: dropping packet\n");
        return 0;
    }

    /* copy the data into it */
    skb_reserve(new_skb, 2);
    memcpy(skb_put(new_skb, len), data, len);

    /* set the device to the other one */
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
    printk(KERN_INFO "loopback local get stats\n");
    return &priv->stats;
}


static const struct header_ops os_header_ops = {
    .create  = os_create_header,
    .cache   = NULL,
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

    for (i=0; i<6; i++) os0->dev_addr[i] = (unsigned char)i;
    for (i=0; i<6; i++) os0->broadcast[i] = (unsigned char)0xFF;
    for (i=0; i<6; i++) os1->dev_addr[i] = (unsigned char)i;
    for (i=0; i<6; i++) os1->broadcast[i] = (unsigned char)0xFF;

    os1->dev_addr[5] = 0x6;

    memcpy(os0->name, "os0", 10);
    memcpy(os1->name, "os1", 10);

    os0->netdev_ops = &os_device_ops;
    os0->header_ops = &os_header_ops;
    os1->netdev_ops = &os_device_ops;
    os1->header_ops = &os_header_ops;

    os0->hard_header_len = 14;
    os1->hard_header_len = 14;

    os0->flags |= IFF_NOARP;
    os1->flags |= IFF_NOARP;

    priv0 = netdev_priv(os0);
    priv1 = netdev_priv(os1);

    memset(priv0, 0, sizeof(struct os_priv));
    memset(priv1, 0, sizeof(struct os_priv));

    priv0->dev = os0;
    priv1->dev = os1;

    priv0->rx_int_enabled = 1;
    priv1->rx_int_enabled = 1;

    priv0->pkt = kmalloc(sizeof(struct os_packet), GFP_KERNEL);
    priv1->pkt = kmalloc(sizeof(struct os_packet), GFP_KERNEL);

    priv0->pkt->dev = os0;
    priv1->pkt->dev = os1;

    spin_lock_init(&priv0->lock);
    spin_lock_init(&priv1->lock);

    if (register_netdev(os0)) printk(KERN_INFO "error registering os0\n");
    else printk(KERN_INFO "os0 registered\n");

    if (register_netdev(os1)) printk(KERN_INFO "error registering os1\n");
    else printk(KERN_INFO "os1 registered\n");

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
        kfree(priv0->pkt);
        unregister_netdev(os0);
    }

    if (os1) {
        priv1 = netdev_priv(os1);
        kfree(priv1->pkt);
        unregister_netdev(os1);
    }

    printk(KERN_INFO "loopback: Module unloaded successfully\n");
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Thrun");
MODULE_DESCRIPTION("Network Loopback");
