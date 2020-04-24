#include <io.h>
#include <init.h>
#include <timer.h>
#include <irq.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <net/net.h>
#include <net/mii.h>

#define EMAC_MPE       (1 << 4)
#define EMAC_IDLE      (1 << 2)

#define RX_BUFF_LEN  128
#define RX_BUFF_NUM  8

// #define TX_BUFF_LEN  128
#define TX_BUFF_NUM  8

#define EMAC_SOF  (1 << 14)
#define EMAC_EOF  (1 << 15)

#define at91_emac_readl(reg) \
	readl(VA(AT91SAM9263_PA_EMAC + (reg)))

#define at91_emac_writel(reg, val) \
	writel(VA(AT91SAM9263_PA_EMAC + (reg)), (val))

struct emac_buff_desc {
	__u32 addr;
	__u32 stat;
};

struct at91_emac {
	struct emac_buff_desc *rx_queue, *rx_head;
	struct emac_buff_desc *tx_queue, *tx_rear;
};

static int at91_emac_recv(struct net_device *);
static int at91_emac_send(struct net_device *, struct sock_buff *);
static int at91_emac_isr(__u32, void *);
static int at91_emac_set_mac(struct net_device *, const __u8 []);

static int at91_emac_isr(__u32 irq, void *dev)
{
	__u32 stat;
	struct net_device *ndev = dev;

	stat = at91_emac_readl(EMAC_ISR);

#ifdef CONFIG_DEBUG
	printf("%s(), isr = 0x%08x\n", __func__, stat);
#endif

	if (stat & 0x2)
		at91_emac_recv(ndev);

	return 0;
}

#ifndef CONFIG_IRQ_SUPPORT
static int at91_emac_poll(struct net_device *ndev)
{
	return at91_emac_isr(0, ndev);
}
#endif

static int at91_emac_send(struct net_device *ndev, struct sock_buff *skb)
{
	__u32 val;
	volatile struct emac_buff_desc *tx_rear;
	__u32 ulFlag;
	struct at91_emac *emac = ndev->chip;

	lock_irq_psr(ulFlag);

	tx_rear = emac->tx_rear++;

	tx_rear->addr = (__u32)skb->data;
	tx_rear->stat = (1 << 15) | skb->size;

	if (emac->tx_rear == emac->tx_queue + TX_BUFF_NUM) {
		tx_rear->stat |= 1 << 30;
		emac->tx_rear = emac->tx_queue;
	}

	val = at91_emac_readl(EMAC_NCR);
	val |= 1 << 9;
	at91_emac_writel(EMAC_NCR, val);

	while (!(tx_rear->stat & (1 << 31))); // fixme

	ndev->stat.tx_packets++;

	unlock_irq_psr(ulFlag);

	return 0;
}

static int at91_emac_recv(struct net_device * ndev)
{
	__u8 *buf_ptr = NULL;
	struct sock_buff *skb;
	struct emac_buff_desc *rx_head, *rx_rear;
	struct at91_emac *emac = ndev->chip;
#ifdef CONFIG_DEBUG
	__u32 count = 0;
#endif

	rx_head = emac->rx_head;
	rx_rear = (struct emac_buff_desc *)at91_emac_readl(EMAC_RBQP);

	assert(rx_head->stat & EMAC_SOF); // fixme

	while (rx_head != rx_rear) {
		if (rx_head->stat & EMAC_SOF) {
			skb = skb_alloc(0, MAX_ETH_LEN);
			// if NULL
			buf_ptr = skb->data;
#ifdef CONFIG_DEBUG
			count++;
#endif
		}

		assert(buf_ptr); // fixme
		memcpy(buf_ptr, (__u8*)(rx_head->addr & ~0x3), RX_BUFF_LEN);
		buf_ptr += RX_BUFF_LEN;

		rx_head->addr &= ~1;

		if (rx_head->stat & EMAC_EOF) {
			skb->size = rx_head->stat & 0xfff;
			netif_rx(skb);
			ndev->stat.rx_packets++;
			buf_ptr = NULL;
		}

		rx_head++;
		if (rx_head == emac->rx_queue + RX_BUFF_NUM)
			rx_head = emac->rx_queue;
	}

#ifdef CONFIG_DEBUG
	if (count > 1)
		printf("%s(): RX frames = %d\n", __func__, count);
#endif

	emac->rx_head = rx_head;

	return 0;
}

static __u16 at91_emac_mdio_read(struct net_device *ndev, __u8 addr, __u8 reg)
{
	__u32 frame;

	frame = (1 << 30) | (2 << 28) | (addr << 23) | (reg << 18) | (2 << 16);
	at91_emac_writel(EMAC_MAN, frame);

	while (!(at91_emac_readl(EMAC_NSR) & EMAC_IDLE));

	frame = at91_emac_readl(EMAC_MAN);

	return (__u16)(frame & 0xffff);
}

static void at91_emac_mdio_write(struct net_device *ndev, __u8 addr, __u8 reg, __u16 val)
{
	at91_emac_writel(EMAC_MAN, (1 << 30) | (1 << 28) | (addr << 23) | (reg << 18) | (2 << 16) | val);

	while (!(at91_emac_readl(EMAC_NSR) & EMAC_IDLE));
}

static int __init at91_emac_init_ring(struct at91_emac *emac)
{
	int i;
	__u32 dma_buff_base;

	// init RX ring buffer
	dma_buff_base = (__u32)malloc(RX_BUFF_LEN * RX_BUFF_NUM);
	// if 0

	emac->rx_head = emac->rx_queue = (struct emac_buff_desc *)malloc(8 * RX_BUFF_NUM);
	// if null

	for (i = 0; i < RX_BUFF_NUM; i++) {
		emac->rx_queue[i].addr = dma_buff_base;
		emac->rx_queue[i].stat = 0;

		dma_buff_base += RX_BUFF_LEN;
	}

	emac->rx_queue[RX_BUFF_NUM - 1].addr |= 0x2;

	// init TX ring buffer
	emac->tx_rear = emac->tx_queue = (struct emac_buff_desc *)malloc(8 * TX_BUFF_NUM);

	for (i = 0; i < TX_BUFF_NUM; i++) {
		emac->tx_queue[i].addr = 0;
		emac->tx_queue[i].stat = 1 << 31;
	}

	emac->tx_queue[TX_BUFF_NUM - 1].stat |= 1 << 30;

	at91_emac_writel(EMAC_RBQP, (__u32)emac->rx_queue);
	at91_emac_writel(EMAC_TBQP, (__u32)emac->tx_queue);

	DPRINT("RX buff = 0x%08x, TX buff = 0x%08x\n",
		emac->rx_queue, emac->tx_queue);

	return 0;
}

static void __init at91_emac_init(struct at91_emac *emac)
{
	__u32 conf;

	at91_emac_init_ring(emac);

	at91_emac_writel(EMAC_NCR, 0);

	at91_emac_writel(EMAC_TSR, ~0UL);
	at91_emac_writel(EMAC_RSR, ~0UL);

	at91_emac_writel(EMAC_IDR, ~0UL);
	at91_emac_readl(EMAC_ISR);

	conf = at91_emac_readl(EMAC_NCFGR);
	conf &= (3 << 10);
	conf |= (1 << 17);
	conf |= (1 << 13);
	conf |= 3;
	at91_emac_writel(EMAC_NCFGR, conf);

	at91_emac_writel(EMAC_NCR, 0x3c);
	at91_emac_writel(EMAC_USRIO, 0x3);
}

static int at91_emac_set_mac(struct net_device *ndev, const __u8 mac_addr[])
{
	// at91_emac_writel(EMAC_SA1B, *(__u32 *)(ndev->mac_addr + 0));
	at91_emac_writel(EMAC_SA1T, *(__u16 *)(ndev->mac_addr + 4));

	return 0;
}

static int __init at91_emac_probe(void)
{
	int ret;
	struct net_device *ndev;
	struct at91_emac *emac;

	ndev = ndev_new(sizeof(*emac));
	emac = ndev->chip;

	//
	ndev->chip_name   = "AT91SAM9263 EMAC";
	//
	ndev->send_packet = at91_emac_send;
	ndev->set_mac_addr = at91_emac_set_mac;
#ifndef CONFIG_IRQ_SUPPORT
	ndev->ndev_poll   = at91_emac_poll;
#endif
	//
	ndev->mdio_read   = at91_emac_mdio_read;
	ndev->mdio_write  = at91_emac_mdio_write;

	at91_clock_enable(PID_EMAC);

	at91_emac_init(emac);

	ret = ndev_register(ndev);
	if (ret < 0) {
		// DPRINT ...
		goto L1;
	}

	irq_register_isr(PID_EMAC, at91_emac_isr, ndev);
	at91_emac_writel(EMAC_IER, 0x3cf7);

	return 0;
L1:
	free(ndev);
	return ret;
}

module_init(at91_emac_probe);

