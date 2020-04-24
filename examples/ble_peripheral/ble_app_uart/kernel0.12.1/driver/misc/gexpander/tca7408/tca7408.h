#pragma once

#define TCA7408_I2C_TYPE		"tca7408"
#define TCA7408_I2C_ADDR		0x43
#define TCA7408_I2C_PORT		0x01
#define TCA7408_ID_NUMBR		0x42

#define TCA_TAG					"[TCA7408]"
#define TCA_LOG(fmt, arg...)	printk(TCA_TAG fmt, ##arg)

#define TCA7408_RESET_PIN		20
#define TCA7408_RESET_HIGH()	gpio_set_out(TCA7408_RESET_PIN, GPIO_OUT_ONE);
#define TCA7408_RESET_LOW()		gpio_set_out(TCA7408_RESET_PIN, GPIO_OUT_ZERO);

#define TCA7408_REG_ID_AND_CTRL		0x01	// Device ID and Control
#define TCA7408_REG_IO_DIRECTION	0x03
#define TCA7408_REG_IO_OUTPUT_VAL	0x05	// Output Port Register
#define TCA7408_REG_IO_OUTPUT_HI	0x07	// Output High-Impedance
#define TCA7408_REG_IO_PULL_ENABL	0x0B	// Pull-Up/-Down Enable
#define TCA7408_REG_IO_PULL_SELET	0x0D	// Pull-Up/-Down Select

#define TCA7408_GPIO_MAX		8