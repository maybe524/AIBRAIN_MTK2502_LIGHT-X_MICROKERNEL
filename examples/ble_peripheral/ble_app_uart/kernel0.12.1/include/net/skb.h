#pragma once

#include "include/types.h"
#include "include/list.h"
#include "include/net/ndev.h"

struct socket;

struct sock_buff {
	__u8  *head;
	__u8  *data;
	__u16  size;

	struct list_head node;
	struct socket *sock;
	struct net_device *ndev;
	// struct sockaddr_in remote_addr;
};

struct sock_buff *skb_alloc(__u32 prot_len, __u32 data_len);

void skb_free(struct sock_buff * skb);
