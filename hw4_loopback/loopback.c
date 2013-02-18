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

#define OS_RX_INTR 0x0001
#define OS_TX_INTR 0x0002

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

void os_rx(struct net_device *dev, struct os_packet *pkt) {
    struct sk_buff *skb;
    struct os_priv *priv = netdev_priv(dev);

    skb = dev_alloc_skb(pkt->datalen + 2);
    if (!skb) {
        printk(KERN_INFO "os_rx: dropping packet\n");
        priv->stats.rx_dropped++;
        goto out;
    }

    /* align IP on 16B boundary */
    skb_reserve(skb, 2);
    memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;

    priv->stats.rx_packets++;
    priv->stats.rx_bytes += pkt->datalen;

    netif_rx(skb);

out:
    return;
}


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
    struct os_priv *priv = netdev_priv(dev);

    struct iphdr *ih;
    struct net_device *dest;
    u32 *saddr, *daddr;
    struct os_priv *priv_dest;
    struct os_packet *tx_buffer;

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

    /* remember the skb so we can free it later */
    priv->skb = skb;

    /* ----------- snull_hw_tx ----------- */

    ih = (struct iphdr *)(data+sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;

    /* toggle the third octet */
    ((u8 *)saddr)[2] ^= 1;
    ((u8 *)daddr)[2] ^= 1;

    /* rebuild the checksum */
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

    if (dev == os0)
        printk(KERN_INFO "%08x:%05i --> %08x:%05i\n",
                ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source),
                ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest));
    else
        printk(KERN_INFO "%08x:%05i <-- %08x:%05i\n",
                ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest),
                ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source));

    /* destination is the other device */
    dest = (dev == os0) ? os1 : os0;

    priv_dest = netdev_priv(dest);
    tx_buffer = priv->pkt;
    tx_buffer->datalen = len;
    memcpy(tx_buffer->data, data, len);
    priv_dest->pkt = tx_buffer;

    spin_lock(&priv_dest->lock);
    os_rx(dest, priv_dest->pkt);
    spin_unlock(&priv_dest->lock);


    priv->tx_packetlen = len;
    priv->tx_packetdata = data;

    spin_lock(&priv->lock);
    priv->stats.tx_packets++;
    priv->stats.tx_bytes += priv->tx_packetlen;
    dev_kfree_skb(priv->skb);
    spin_unlock(&priv->lock);


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
