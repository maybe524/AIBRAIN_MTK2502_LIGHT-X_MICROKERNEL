#pragma once

#include <include/list.h>
#include <include/stdio.h>
#include <include/types.h>

struct spi_slave;

struct spi_master {
	char *name;
	int bus_num;
	const void	*platform_data;

	kd_bool lock;

	struct spi_slave *active_slave;
	struct list_head slave_list;

	int (*spi_transfer)(struct spi_slave *);
};

struct spi_slave_info;
struct spi_slave {
	char *name;
	struct list_head slave_node;
	struct spi_master *master;
	struct list_head msg_qu;

	struct spi_slave_info *slave_info;
};

struct spi_trans_msg {
	struct list_head msg_node;
	const __u8 *tx_buf;
	__u8 *rx_buf;
	__u32 len;
};

struct spi_config {
	__u8 bus_num;
    __u8 sck_pin;
    __u8 mosi_pin;
    __u8 miso_pin;
    __u8 ss_pin;
    __u8 irq_priority;
    __u8 orc;
    __u32 frequency;
    __u32 mode;
    __u8 bit_order;
};

struct spi_bus_info {
	const char *name;
	const struct spi_config *confg;
	struct list_head	list;

	int (*prode)(void);
};

#define SPI_NAME_SIZE		8

/*---------------------------------------------------------------------------*/

/*
 * INTERFACE between board init code and SPI infrastructure.
 *
 * No SPI driver ever sees these SPI device table segments, but
 * it's how the SPI core (or adapters that get hotplugged) grows
 * the driver model tree.
 *
 * As a rule, SPI devices can't be probed.  Instead, board init code
 * provides a table listing the devices which are present, with enough
 * information to bind and set up the device's driver.  There's basic
 * support for nonstatic configurations too; enough to handle adding
 * parport adapters, or microcontrollers acting as USB-to-SPI bridges.
 */

/**
 * struct spi_board_info - board-specific template for a SPI device
 * @modalias: Initializes spi_device.modalias; identifies the driver.
 * @platform_data: Initializes spi_device.platform_data; the particular
 *	data stored there is driver-specific.
 * @controller_data: Initializes spi_device.controller_data; some
 *	controllers need hints about hardware setup, e.g. for DMA.
 * @irq: Initializes spi_device.irq; depends on how the board is wired.
 * @max_speed_hz: Initializes spi_device.max_speed_hz; based on limits
 *	from the chip datasheet and board-specific signal quality issues.
 * @bus_num: Identifies which spi_master parents the spi_device; unused
 *	by spi_new_device(), and otherwise depends on board wiring.
 * @chip_select: Initializes spi_device.chip_select; depends on how
 *	the board is wired.
 * @mode: Initializes spi_device.mode; based on the chip datasheet, board
 *	wiring (some devices support both 3WIRE and standard modes), and
 *	possibly presence of an inverter in the chipselect path.
 *
 * When adding new SPI devices to the device tree, these structures serve
 * as a partial device template.  They hold information which can't always
 * be determined by drivers.  Information that probe() can establish (such
 * as the default transfer wordsize) is not included here.
 *
 * These structures are used in two places.  Their primary role is to
 * be stored in tables of board-specific device descriptors, which are
 * declared early in board initialization and then used (much later) to
 * populate a controller's device tree after the that controller's driver
 * initializes.  A secondary (and atypical) role is as a parameter to
 * spi_new_device() call, which happens after those controller drivers
 * are active in some dynamic board configuration models.
 */
struct spi_slave_info {
	/* the device name and module name are coupled, like platform_bus;
	 * "modalias" is normally the driver name.
	 *
	 * platform_data goes to spi_device.dev.platform_data,
	 * controller_data goes to spi_device.controller_data,
	 * irq is copied too
	 */
	char		*modalias;
	char 	*matchto_master_name;
	const void	*platform_data;
	//void		*controller_data;
	//int		irq;

	/* slower signaling on noisy or low voltage boards */
	u32		max_speed_hz;


	/* bus_num is board specific and matches the bus_num of some
	 * spi_master that will probably be registered later.
	 *
	 * chip_select reflects how this chip is wired to that master;
	 * it's less than num_chipselect.
	 */
	u8		bus_num;
	u16		chip_select;
	u16		dc_select;

	/* mode becomes spi_device.mode, and is essential for chips
	 * where the default of SPI_CS_HIGH = 0 is wrong.
	 */
	u8		mode;

	/* ... may need additional spi_device chip config data here.
	 * avoid stuff protocol drivers can set; but include stuff
	 * needed to behave without being bound to a driver:
	 *  - quirks like clock rate mattering when not selected
	 */

	u8		status;
};

/**
 * spi_w8r8 - SPI synchronous 8 bit write followed by 8 bit read
 * @spi: device with which data will be exchanged
 * @cmd: command to be written before data is read back
 * Context: can sleep
 *
 * This returns the (unsigned) eight bit number returned by the
 * device, or else a negative error code.  Callable only from
 * contexts that can sleep.
 */
static inline ssize_t spi_w8r8(struct spi_slave *spi, u8 cmd)
{
	ssize_t			status;
	u8			result;

	status = spi_write_then_read(spi, &cmd, 1, &result, 1);

	/* return negative errno or unsigned value */
	return (status < 0) ? status : result;
}

/**
 * spi_w8r16 - SPI synchronous 8 bit write followed by 16 bit read
 * @spi: device with which data will be exchanged
 * @cmd: command to be written before data is read back
 * Context: can sleep
 *
 * This returns the (unsigned) sixteen bit number returned by the
 * device, or else a negative error code.  Callable only from
 * contexts that can sleep.
 *
 * The number is returned in wire-order, which is at least sometimes
 * big-endian.
 */
static inline ssize_t spi_w8r16(struct spi_slave *spi, u8 cmd)
{
	ssize_t			status;
	u16			result;

	status = spi_write_then_read(spi, &cmd, 1, (u8 *) &result, 2);

	/* return negative errno or unsigned value */
	return (status < 0) ? status : result;
}

struct spi_master *spi_master_alloc();
struct spi_slave  *spi_slave_alloc();
int spi_slave_attach(struct spi_master *, struct spi_slave*);
int spi_master_register(struct spi_master *);
int spi_write_then_read(struct spi_slave *slave, const __u8 *txbuf, __u32 n_tx, __u8 *rxbuf, __u32 n_rx);
int spi_transfer(struct spi_slave *slave);
struct spi_slave *get_spi_slave(char *name);
struct spi_master *get_spi_master(char *name);
int set_master_tab(struct spi_master spi_master[], int len);
int set_slave_tab(struct spi_slave spi_slave[], int len);
struct spi_slave *spi_register_slave_info(struct spi_slave_info *info, unsigned n);

#define spi_err(fmt, argc...)	printk("SPI ERROR:"fmt, ##argc)


#define SPI_EXOPTS_ATTR_COMMAND		0x01
#define SPI_EXOPTS_ATTR_PARAMETER	0x02

struct spi_exopts {
	int (*exopt_prexfer)(struct spi_exopts *exopt);
	int (*exopt_afterxfer)(struct spi_exopts *exopt);

	__u8 exopt_attr;
};

#define SPI_UNSUE_PIN	0xFF
#define SPI_WRITE_CMD	0x01
#define SPI_WRITE_DAT	0x02
#define SPI_READ_BYTE	0x03

#define SPI_MODE_3WIRE_1DAT_0DC		0x01		/* 1 data, 1 clk, 1 cs */
#define SPI_MODE_4WIRE_1DAT_1DC		0x02		/* 1 data, 1 dc, 1 clk, 1 cs */