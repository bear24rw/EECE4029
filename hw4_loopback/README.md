Network Loopback Driver
=====================

This module implements a network loopback in the form of two network devices,
`os0` and `os1`.  Packets sent out `os0` to `remote0` will be received by `os1`
from `remote1` and vica versa.  It implements the absolute minimum nessesary to
achieve the same basic functionality of the `snull` device driver found in
chapter 14 of [Linux Device Drivers](http://www.xml.com/ldd/chapter/book/).

Network Setup
-------------
An overview of the network we are trying to create, as described in the book,
is shown below:

![network](http://www.xml.com/ldd/chapter/book/figs/ldr2_1401.gif "network")

Any packets sent from `local0` to `remote0` get looped back and sent to `local1`
from `remote1`.  The same happens in reverse with packets sent from `local1`.
This creates the illusion that `local0` is talking to `remote0` when in reality
it is talking to `local1`. `local1` thinks that it is getting a packet from
`remote1` when in reality that packet was sent from `local0`.

In order to help visualize these transfers we can enter the following hosts
into `/etc/hosts`

```
10.0.0.1 	local0
10.0.0.2 	remote0
10.0.1.2 	local1
10.0.1.1 	remote1
```

Note that it is important that the host portion (last octet) of `local0` is the
same as `remote1`, and the host portion of `local1` is the same as `remote0`.
The module requires this relationship to be true for it to successfully
loopback the packet.

The network interfaces can be brought up using:

```
sudo ifconfig os0 local0 netmask 255.255.255.0 broadcast 10.0.0.255
sudo ifconfig os1 local1 netmask 255.255.255.0 broadcast 10.0.1.255
```

Demonstration
-------------
In order to demonstrate the functionality of the module we can attempt to ping
one of the virtual remote hosts.

```
user@gentoovm ~/EECE4029/hw4_loopback (master) % ping remote0
PING remote0 (10.0.0.2) 56(84) bytes of data.
64 bytes from remote0 (10.0.0.2): icmp_seq=1 ttl=64 time=0.026 ms
64 bytes from remote0 (10.0.0.2): icmp_seq=2 ttl=64 time=0.038 ms
64 bytes from remote0 (10.0.0.2): icmp_seq=3 ttl=64 time=0.052 ms
64 bytes from remote0 (10.0.0.2): icmp_seq=4 ttl=64 time=0.039 ms
```

We can see that `remote0` appears to be there as it is responding to our pings.
By using `tcpdump` we can see what is happening behind the scenes. First we
will monitor `os0`:

```
user@gentoovm ~/tcpdump (master) % sudo ./tcpdump -i os0
listening on os0, link-type EN10MB (Ethernet), capture size 65535 bytes
19:18:43.749826 IP local0 > remote0: ICMP echo request, id 15267, seq 5, length 64
19:18:43.749845 IP remote0 > local0: ICMP echo reply, id 15267, seq 5, length 64
```

We can see that there is a ping request from `local0` to the virtual remote
host `remote0` and that `remote0` replies to the ping as expected. If we take a
look at `os1` now:

```
user@gentoovm ~/tcpdump (master) % sudo ./tcpdump -i os1
listening on os1, link-type EN10MB (Ethernet), capture size 65535 bytes
19:23:38.101464 IP remote1 > local1: ICMP echo request, id 15267, seq 297, length 64
19:23:38.101478 IP local1 > remote1: ICMP echo reply, id 15267, seq 297, length 64
```

we can see that the ping request is apparently coming from `remote1` to
`local1` and `local1` responds as expected. This demonstrates that the driver is
properly relaying the packets.

Additionally, if we look at dmesg we can see that at the ethernet layer the ping
request is being sent from `os0`, whose MAC address ends in 5, to `os1`, whose
MAC address ends in 6.  At the IP layer, however, we can see that the source IP
now looks like it is coming from `remote1` and is going to `local1`. This is
because the driver flips the 3rd octet of the IP in order to do the network
crossover.  This explains why we needed to have a particular relationship
between the IP addresses as explained earlier.

```
Feb 18 19:30:05 gentoovm kernel: loopback: created packet from 0:1:2:3:4:5 --> 0:1:2:3:4:6
Feb 18 19:30:05 gentoovm kernel: loopback: sending packet from 10.0.1.1 --> 10.0.1.2
```

The ping reply shows the opposite:

```
Feb 18 19:30:05 gentoovm kernel: loopback: created packet from 0:1:2:3:4:6 --> 0:1:2:3:4:5
Feb 18 19:30:05 gentoovm kernel: loopback: sending packet from 10.0.0.2 --> 10.0.0.1
```

