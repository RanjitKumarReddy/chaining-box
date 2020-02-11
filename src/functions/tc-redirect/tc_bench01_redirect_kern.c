/*  TC (Traffic Control) eBPF redirect benchmark
 *
 *  NOTICE: TC loading is different from XDP loading. TC bpf objects
 *          use the 'tc' cmdline tool from iproute2 for loading and
 *          attaching bpf programs.
 *
 *  Copyright(c) 2017 Jesper Dangaard Brouer, Red Hat Inc.
 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/pkt_cls.h>

#include "cb_helpers.h"
#include "bpf_endian.h"
#include "bpf_helpers.h"

/* Notice: TC and iproute2 bpf-loader uses another elf map layout */
struct bpf_elf_map {
	__u32 type;
	__u32 size_key;
	__u32 size_value;
	__u32 max_elem;
	__u32 flags;
	__u32 id;
	__u32 pinning;
};

/* TODO: Describe what this PIN_GLOBAL_NS value 2 means???
 *
 * A file is automatically created here:
 *  /sys/fs/bpf/tc/globals/egress_ifindex
 */
#define PIN_GLOBAL_NS	2

struct bpf_elf_map SEC("maps") egress_ifindex = {
	.type = BPF_MAP_TYPE_ARRAY,
	.size_key = sizeof(int),
	.size_value = sizeof(int),
	.pinning = PIN_GLOBAL_NS,
	.max_elem = 1,
};

struct bpf_elf_map SEC("maps") srcip = {
	.type = BPF_MAP_TYPE_ARRAY,
	.size_key = sizeof(int),
	.size_value = sizeof(int),
	.pinning = PIN_GLOBAL_NS,
	.max_elem = 1,
};

static void swap_src_dst_mac(void *data)
{
	unsigned short *p = data;
	unsigned short dst[3];

	dst[0] = p[0];
	dst[1] = p[1];
	dst[2] = p[2];
	p[0] = p[3];
	p[1] = p[4];
	p[2] = p[5];
	p[3] = dst[0];
	p[4] = dst[1];
	p[5] = dst[2];
}

/* Notice this section name is used when attaching TC filter
 *
 * Like:
 *  $TC qdisc   add dev $DEV clsact
 *  $TC filter  add dev $DEV ingress bpf da obj $BPF_OBJ sec ingress_redirect
 *  $TC filter show dev $DEV ingress
 *  $TC filter  del dev $DEV ingress
 *
 * Does TC redirect respect IP-forward settings?
 *
 */
SEC("ingress_redirect")
int _ingress_redirect(struct __sk_buff *skb)
{
  cb_debug("Starting TC redirect\n");

	void *data     = (void *)(long)skb->data;
	void *data_end = (void *)(long)skb->data_end;
	struct ethhdr *eth = data;
	int key = 0, *ifindex;
    __u32 *sip;

	if (data + sizeof(*eth) > data_end)
		return TC_ACT_OK;

	/* Keep ARP resolution working */
	if (eth->h_proto == bpf_htons(ETH_P_ARP))
		return TC_ACT_OK;

	/* Lookup what ifindex to redirect packets to */
	ifindex = bpf_map_lookup_elem(&egress_ifindex, &key);
	if (!ifindex)
		return TC_ACT_OK;

  cb_debug("ifi OK. Checking sip...\n");

  struct iphdr *ip = (struct iphdr*)(data + sizeof(struct ethhdr));
  if((void*)ip + sizeof(struct iphdr) > data_end)
    return TC_ACT_OK;

  sip = bpf_map_lookup_elem(&srcip, &key);
  if (!sip)
    return TC_ACT_OK;

  if(ip->protocol == 1){
    cb_debug("IT'S A PING!!!\n");
  }

  if(*sip != ip->saddr){
    cb_debug("sip check failed. Passing along... %x != %x\n",*sip,ip->saddr);
    return TC_ACT_OK;
  }else{
    cb_debug("IT'S A MATCH!!!\n");
  }

  /* Swap src and dst mac-addr if ingress==egress
   * --------------------------------------------
   * If bouncing packet out ingress device, we need to update
   * MAC-addr, as some NIC HW will drop such bounced frames
   * silently (e.g mlx5).
   *
   * Note on eBPF translations:
   *  __sk_buff->ingress_ifindex == skb->skb_iif
   *   (which is set to skb->dev->ifindex, before sch_handle_ingress)
   *  __sk_buff->ifindex == skb->dev->ifindex
   *   (which is translated into BPF insns that deref dev->ifindex)
   */
  // if (*ifindex == skb->ingress_ifindex)
    // swap_src_dst_mac(data);

  //return bpf_redirect(*ifindex, BPF_F_INGRESS); // __bpf_rx_skb
  return bpf_redirect(*ifindex, 0); // __bpf_tx_skb / __dev_xmit_skb
}

char _license[] SEC("license") = "GPL";

