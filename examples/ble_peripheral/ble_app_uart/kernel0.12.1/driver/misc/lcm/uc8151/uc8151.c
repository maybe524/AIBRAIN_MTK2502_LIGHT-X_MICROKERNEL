#include <include/stdio.h>
#include <include/errno.h>
#include <include/lcm/lcm_drv.h>
#include <include/gpio.h>
#include <include/delay.h>
#include <include/spi.h>
#include <include/errno.h>
#include <include/lock.h>
#include <include/lcm/lcm_drv.h>
#include <include/assert.h>
#include <include/fb.h>

#include "uc8151.h"
#include "uc8151_background_default.h"
#include "bq25120.h"

#define UC8151_LOG(fmt, argc...)	printk("[UC8151]: "fmt , ##argc)

// #define CONFIG_BOARD_OLD

#ifdef CONFIG_BOARD_OLD
#define UC8151_CS_PIN		EXGPIO_6
#define UC8151_RST_PIN		EXGPIO_7
#else
#define UC8151_CS_PIN		GPIO_22
#define UC8151_RST_PIN		GPIO_19
#endif


#define UC8151_REST_HIGH()		gpio_set_out(UC8151_RST_PIN, GPIO_OUT_ONE);
#define UC8151_REST_LOW()		gpio_set_out(UC8151_RST_PIN, GPIO_OUT_ZERO);

// #define UC8151_DEBUG

#define USPI_AUTO_MODE		0x00
#define USPI_WCMD_MODE		0x01
#define USPI_WDAT_MODE		0x02
#define USPI_WODT_MODE		0x04
#define USPI_VOID_BUSY		0x80

struct init_reg {
	__u8	len;
	__u8	reg_val[50];
};

struct uc8151_info {
	struct spi_slave	*slave;
	struct spi_slave_info	*slave_info;
};

static struct uc8151_info uc8151_infos = {0};
static const struct init_reg uc8151_init_reg[] = {
	#include "uc8151_init_reg.h"
};
static int tempbakup = -1;
static kd_bool uc8151_is_oppositecolor_mode = kd_false;

static int uc8151_spi_read(__u8 *rxbuf, __u32 n_rx, __u8 flags)
{
	int ret;
	struct uc8151_info *uc8151 = &uc8151_infos;
	
	if (!rxbuf || !n_rx)
		return 0;
	
	for (int cnt = 0; cnt < n_rx; cnt++) {
		__u8 txbuf[1] = {0};
		__u32 timeout = 150000;
		while (!gpio_get_val(GPIO_18) || check_lock(&uc8151->slave->master->lock)) {
			timeout--;
			if (!timeout) {
				UC8151_LOG("rBusy: %d, Master %s lock.\r\n", 
						gpio_get_val(GPIO_18), check_lock(&uc8151->slave->master->lock) ? "is" : "isn't");
				return -ETIMEDOUT;
			}
			sleep_on(NULL);
		}

		uc8151->slave_info->status = SPI_READ_BYTE;
		ret = spi_write_then_read(uc8151->slave, txbuf, 1, rxbuf + cnt, 1);
	}

	return ret;
}

static int uc8151_spi_write(const __u8 *txbuf, __u32 n_tx, __u8 flags)
{
	int ret;
	struct uc8151_info *uc8151 = &uc8151_infos;
	__u8 txchar;

	if (!n_tx || !txbuf)
		return 0;

	for (int cnt = 0; cnt < n_tx; cnt++) {
		__u8 rxbuf[1];
		__u32 timeout = 150000;
		while (!gpio_get_val(GPIO_18) || check_lock(&uc8151->slave->master->lock)) {
			if (flags & USPI_VOID_BUSY)
				break;
			timeout--;
#ifdef UC8151_DEBUG
			UC8151_LOG("wBusy: %d, lock: %d, timeout: %d\r\n", gpio_get_val(GPIO_18), uc8151->slave->master->lock, timeout);
#endif
			if (!timeout) {
				UC8151_LOG("Busy: %d, Master %s lock.\r\n", 
						gpio_get_val(GPIO_18), check_lock(&uc8151->slave->master->lock) ? "is" : "isn't");
				return -ETIMEDOUT;
			}
			sleep_on(NULL);
		}

		if (flags & USPI_WCMD_MODE)
			uc8151->slave_info->status = SPI_WRITE_CMD;
		else if (flags & USPI_WDAT_MODE)
			uc8151->slave_info->status = SPI_WRITE_DAT;
		else
			uc8151->slave_info->status = cnt ? SPI_WRITE_DAT : SPI_WRITE_CMD;

		if (uc8151->slave_info->status == SPI_WRITE_DAT && \
				(flags & USPI_WODT_MODE || uc8151_is_oppositecolor_mode))
			txchar = ~txbuf[cnt];
		else
			txchar =  txbuf[cnt];
		ret = spi_write_then_read(uc8151->slave, &txchar, 1, rxbuf, 0);
	}

	return ret;
}

static int uc8151_set_partlut(void)
{
	int ret;
	__u8 tx_buf[10] = {0}, rx_buf[10] = {0};
	__u8 tempraw, tempfull = 0, temppartial;

	tx_buf[0] = 0x40;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	ret = uc8151_spi_read(&tempraw, 1, 0);

	if (tempbakup < 0)
		tempbakup = tempraw;

	tempfull = tempraw;
	if(64 < tempfull && tempfull < 128)
		tempfull = 63;
	else if(tempfull > 128)					
		tempfull = 1;							
	
	temppartial = tempfull + 64;
	if(temppartial > 128)
		temppartial = 127;

	UC8151_LOG("tempraw = %03d, tempfull = %03d, temppartial = %03d.\r\n", tempraw, tempfull, temppartial);

	tx_buf[0] = 0xe0;
	tx_buf[1] = 0x02;
	ret = uc8151_spi_write(tx_buf, 2, USPI_AUTO_MODE);

	tx_buf[0] = 0xe5;
	tx_buf[1] = temppartial;
	ret = uc8151_spi_write(tx_buf, 2, USPI_AUTO_MODE);

	return ret;
}

static int uc8151_update(void *buff, __u32 x, __u32 y, __u32 width, __u32 height, __u32 flags)
{
	int ret;
	__u8 tx_buf[10] = {0};
	__u8 source_start, source_end;
	__u16 gate_start, gate_end;
	__u32 size;
	__u8 *buf1, *buf2;

	UC8151_LOG("%p, X:%d, Y:%d, W:%d, H:%d, F:0x%x.\r\n", buff, x, y, width, height, flags);

	if (flags == (FB_REGION_FLAGS_USEBUFF | FB_REGION_FLAGS_UPDATEFULL))
		goto update_full;
	else if (flags & FB_REGION_FLAGS_USEBUFF || flags & FB_REGION_FLAGS_USEBASE)
		goto update_part;
	else
		assert(0);

update_part:
	source_start = x;
	source_end = x + width;
	gate_start = y;
	gate_end = y + height;
	size = width * height;

	//if (((source_start) % 8) || ((source_end) % 8))
	//	return -ENOTSUPP;

	uc8151_set_partlut();

	tx_buf[0] = 0x91;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);

	tx_buf[0] = 0x90;
	tx_buf[1] = source_start;			// Source start
	tx_buf[2] = source_end;			// Source end
	tx_buf[3] = gate_start / 256;			// Gate start byte1
	tx_buf[4] = gate_start % 256;		// Gate start byte2
	tx_buf[5] = gate_end / 256;			// Gate end byte1
	tx_buf[6] = gate_end % 256;		// Gate end byte2
	ret = uc8151_spi_write(tx_buf, 7, USPI_AUTO_MODE);

	tx_buf[0] = 0x10;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	ret = uc8151_spi_write((__u8 *)buff, size, USPI_WDAT_MODE | USPI_WODT_MODE);
	tx_buf[0] = 0x13;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	ret = uc8151_spi_write((__u8 *)buff, size, USPI_WDAT_MODE);

	tx_buf[0] = 0x12;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	tx_buf[0] = 0x92;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);

	tx_buf[0] = 0xe5;
	tx_buf[1] = tempbakup;
	ret = uc8151_spi_write(tx_buf, 2, USPI_AUTO_MODE);
	tempbakup = -1;

	return size;

update_full:
	size = width * height;

	struct full_update_buff {
		__u8 *buff_r10;
		__u8 *buff_r13;
	} *full_buff = (struct full_update_buff *)buff;

	tx_buf[0] = 0x10;
	uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	uc8151_spi_write(full_buff->buff_r10, size, USPI_WDAT_MODE);
	tx_buf[0] = 0x13;
	uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	uc8151_spi_write(full_buff->buff_r13, size, USPI_WDAT_MODE);

	tx_buf[0] = 0x12;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);

	return size;
}

int uc8151_fillregion_middle(int idx, char font)
{
	int ret , i, j, n;
	__u8 tx_buf[100] = {0};
	__u32 color = 0;

	tx_buf[0] = 0x91;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	tx_buf[0] = 0x90;
	tx_buf[1] = 32;		//Source start
	tx_buf[2] = 47-1;	//Source end
	tx_buf[3] = 0;		//Gate start byte1
	tx_buf[4] = idx * 24;		//Gate start byte2
	tx_buf[5] = 0;		//Gate end byte1
	tx_buf[6] = (idx + 1) * 24;	//Gate end byte2
	ret = uc8151_spi_write(tx_buf, 7, USPI_AUTO_MODE);

	tx_buf[0] = 0x10;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	ret = uc8151_spi_write(&mould1624[16 + font][0], 48, USPI_WDAT_MODE);
	tx_buf[0] = 0x13;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	ret = uc8151_spi_write(&mould1624[16 + font][0], 48, USPI_WDAT_MODE);

	tx_buf[0] = 0x12;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);
	tx_buf[0] = 0x92;
	ret = uc8151_spi_write(tx_buf, 1, USPI_AUTO_MODE);

}

static int uc8151_background_default(int id)
{
	int ret;
	__u8 tx_buf[10] = {0};

	tx_buf[0] = 0x10;
	uc8151_spi_write(tx_buf, 1, SPI_WRITE_CMD);
	uc8151_spi_write(background_deault_r10, sizeof(background_deault_r10), SPI_WRITE_DAT);
	tx_buf[0] = 0x13;
	uc8151_spi_write(tx_buf, 1, SPI_WRITE_CMD);
	uc8151_spi_write(background_deault_r13, sizeof(background_deault_r13), SPI_WRITE_DAT);
	
	tx_buf[0] = 0x12;
	ret = uc8151_spi_write(tx_buf, 1, SPI_WRITE_CMD);

	return 0;
}

static int uc8151_local_init(void)
{
	int ret;
	struct uc8151_info *uc8151 = &uc8151_infos;
	int data_len = ARRAY_ELEM_NUM(uc8151_init_reg);

	UC8151_REST_HIGH();
	mdelay(5);
	UC8151_REST_LOW();
	mdelay(100);
	UC8151_REST_HIGH();
	mdelay(5);

	for (int idx = 0; idx < data_len; idx++) {
		if (uc8151_init_reg[idx].len == 0xFF)
			mdelay(uc8151_init_reg[idx].reg_val[0]);
		else {
			UC8151_LOG("idx = %02d, len = %d\r\n", idx, uc8151_init_reg[idx].len);
			ret = uc8151_spi_write(&uc8151_init_reg[idx].reg_val[0], uc8151_init_reg[idx].len, USPI_AUTO_MODE);
		}
	}

	UC8151_LOG("uc8151 Reg init %s\r\n", ret ? "FAIL" : "OK");

	uc8151_background_default(0);

	return ret;
}

static int uc8151_suspend(void)
{
	__u8 tx_buf[10] = {0};

	tx_buf[0] = 0x50;
	tx_buf[1] = 0x17;
	uc8151_spi_write(tx_buf, 2, USPI_AUTO_MODE);

	tx_buf[0] = 0x82;
	tx_buf[1] = 0x00;
	uc8151_spi_write(tx_buf, 2, USPI_AUTO_MODE);
	mdelay(1000);

	tx_buf[0] = 0x01;
	tx_buf[1] = 0x00;
	tx_buf[2] = 0x00;
	tx_buf[3] = 0x00;
	tx_buf[4] = 0x00;
	tx_buf[5] = 0x00;
	uc8151_spi_write(tx_buf, 6, USPI_AUTO_MODE);
	mdelay(1000);

	tx_buf[0] = 0x02;
	uc8151_spi_write(tx_buf, 1, USPI_WCMD_MODE);
	tx_buf[0] = 0x07;
	tx_buf[1] = 0xA5;
	uc8151_spi_write(tx_buf, 2, USPI_AUTO_MODE | USPI_VOID_BUSY);
	mdelay(5000);

	bq25120_set_sysout(BQ_SYSOUT_1_1V);

	return 0;
}

static int uc8151_resume(void)
{
	return 0;
}

static int uc8151_get_info(struct lcd_info *info)
{
	info->model = info->vmode.model= "uc8151_lcm_drv";
	info->pix_format = PIX_BIT2;
	info->vmode.height = 184;
	info->vmode.width  = 88;
	info->vmode.hfp = 0;
	info->vmode.hbp = 0;
	info->vmode.hpw = 0;
	info->vmode.vfp = 0;
	info->vmode.vbp = 0;
	info->vmode.vpw = 0;

	return 0;
}

static void uc8151_gpio_init(void)
{
	bq25120_set_sysout(BQ_SYSOUT_3_3V);
	mdelay(1);

	// CS
	gpio_set_dir(UC8151_CS_PIN, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);
	gpio_set_out(UC8151_CS_PIN, GPIO_OUT_ONE);

	// RES#
	gpio_set_dir(UC8151_RST_PIN, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE | GPIO_CFG_PULLUP);
	gpio_set_out(UC8151_RST_PIN, GPIO_OUT_ONE);

	// D/C#	
	gpio_set_dir(GPIO_26, GPIO_DIR_OUT, 0);
	gpio_set_out(GPIO_26, GPIO_OUT_ONE);

	// Busy
	gpio_set_dir(GPIO_18, GPIO_DIR_IN, 0);

	mdelay(50);
}

static struct spi_slave_info uc8151_slave_info = {
	.modalias = "uc8151_spi1",
	.matchto_master_name = "nrfs_spi1_master",
	.bus_num = 0x01,
	.dc_select = GPIO_26,
	.chip_select = UC8151_CS_PIN,
	.mode = SPI_MODE_4WIRE_1DAT_1DC,
};

int uc8151_init(void)
{
	int ret;
	struct uc8151_info *uc8151 = &uc8151_infos;

	UC8151_LOG("uc8151 init\r\n");

	uc8151_gpio_init();

	uc8151->slave = spi_register_slave_info(&uc8151_slave_info, 1);
	if (!uc8151->slave)
		return -ENOMEM;
	uc8151->slave_info = &uc8151_slave_info;

	ret = uc8151_local_init();
	UC8151_LOG("uc8151 init %s\r\n", ret ? "FAIL" : "OK");

	return ret;
}

DECLARE_LCM_DRV_INIT(uc8151_init);
DECLARE_LCM_DRV_SUSPEND(uc8151_suspend);
DECLARE_LCM_DRV_RESUME(uc8151_resume);
DECLARE_LCM_DRV_UPDATE(uc8151_update);
DECLARE_LCM_DRV_GETINFO(uc8151_get_info);
