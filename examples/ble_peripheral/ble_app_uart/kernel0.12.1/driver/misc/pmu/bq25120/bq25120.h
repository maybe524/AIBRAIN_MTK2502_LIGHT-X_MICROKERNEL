#pragma once

#define BQ_TAG		"[BQ25120]"
#define BQ_LOG(fmt, arg...)		printk(BQ_TAG fmt, ##arg)

#define BQ_INT_PIN					8
#define BQ_LCSCTRL_ENABLE_PIN		9
#define BQ_CD_ENABLE_PIN			10
#define BQ_MR_ENABLE_PIN			23
#define BQ_TO_BLE_RESET_PIN			21

#define BQ25120_I2C_TYPE		"bq25120"
#define BQ25120_I2C_ADDR		0x6A
#define BQ25120_I2C_PORT		0x01

#define BQ25120_REG_SCTRL		0x00		// Status and Ship Mode Control Register
#define BQ25120_REG_TSCTRL		0x01		// TS Control and Faults Masks Register
#define BQ25120_REG_FSCTRL		0x02		// Fast Charge Control Register
#define BQ25120_REG_FAST_CHARGE	0x03		// Fast Charge Control Register
#define BQ25120_REG_PRECHG		0x04		// Termination/Pre-Chargeand I2C Address Register
#define BQ25120_REG_BAT_VOLCTL	0x05		// Battery Voltage Control Register
#define BQ25120_REG_SYSOUT_CTRL	0x06
#define BQ25120_REG_LDO_CTRL	0x07
#define BQ25120_REG_PBUTTCTRL	0x08		// Push-button Control Register
#define BQ25120_REG_BUVLOCTRL	0x09		// ILIM and Battery UVLO Control Register
#define BQ25120_REG_VBBM		0x0A		// Voltage Based Battery Monitor Register
#define BQ25120_REG_VINDPM		0x0B		// VIN_DPM and Timers Register
#define BQ25120_REG_MAX			0x0C

// SYSOUT setting
#define BQ_SYSOUT_0_0V		(0xFF << 0)
#define BQ_SYSOUT_1_1V		(0x00 << 1)
#define BQ_SYSOUT_1_2V		(0x01)
#define BQ_SYSOUT_1_8V		(0x15)
#define BQ_SYSOUT_2_8V		(0x1F)
#define BQ_SYSOUT_3_3V		(0x3F << 1)

// LDO setting
#define BQ_LDOOUT_0_0V		(0xFF)
#define BQ_LDOOUT_0_1V		(0x04)
#define BQ_LDOOUT_0_2V		(0x08)
#define BQ_LDOOUT_0_4V		(0x10)
#define BQ_LDOOUT_0_8V		(0x20)
#define BQ_LDOOUT_1_6V		(0x40)
#define BQ_LDOOUT_3_1V		(0x17 << 2)

// Charge current
#define BQ_ICHRGE_00_0MA	(0xFF)
#define BQ_ICHRGE_01_0MA	(0x01 << 2)
#define BQ_ICHRGE_02_0MA	(0x02 << 2)
#define BQ_ICHRGE_04_0MA	(0x04 << 2)
#define BQ_ICHRGE_08_0MA	(0x08 << 2)
#define BQ_ICHRGE_16_0MA	(0x10 << 2)
#define BQ_ICHRGE_90_0MA	(0x25 << 2)

#define BQ_ICHRGE_CE_DISENABLE		(0x00 << 1)
#define BQ_ICHRGE_CE_ENABLE			(0x01 << 1)

#define BQ_HIZ_MODE					0x01
#define BQ_ACTIVE_BATTERY_MODE		0x02

#define BQ_STAT_READY			0x00
#define BQ_STAT_CHARGING		0x01
#define BQ_STAT_CHARGE_DONE		0x02
#define BQ_STAT_FAULT			0x03

#define BQ_VINFAULT_OVER		0x02
#define BQ_VINFAULT_UNDR		0x01

#define BQ_SET_CD_HIGH()		gpio_set_out(BQ_CD_ENABLE_PIN, GPIO_OUT_ONE);
#define BQ_SET_CD_LOW()			gpio_set_out(BQ_CD_ENABLE_PIN, GPIO_OUT_ZERO);

#define BQ_DEBUG_OPEN			(1 << 0)


int bq25120_set_sysout(__u8 val);
int bq25120_lock_ldout(void);
int bq25120_unlock_ldout(void);
