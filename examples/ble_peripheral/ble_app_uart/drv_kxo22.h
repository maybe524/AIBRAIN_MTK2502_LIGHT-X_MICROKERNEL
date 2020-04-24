#ifndef _drv_KX022_H_
#define _drv_KX022_H_

#include <stdint.h>

// KXTI9 register addresses
#define ACC_REG_ADDR_XOUT_HPF_L        0x00 // R
#define ACC_REG_ADDR_XOUT_HPF_H        0x01 // R
#define ACC_REG_ADDR_YOUT_HPF_L        0x02 // R
#define ACC_REG_ADDR_YOUT_HPF_H        0x03 // R
#define ACC_REG_ADDR_ZOUT_HPF_L        0x04 // R
#define ACC_REG_ADDR_ZOUT_HPF_H        0x05 // R
#define ACC_REG_ADDR_XOUT_L            0x06 // R
#define ACC_REG_ADDR_XOUT_H            0x07 // R
#define ACC_REG_ADDR_YOUT_L            0x08 // R
#define ACC_REG_ADDR_YOUT_H            0x09 // R
#define ACC_REG_ADDR_ZOUT_L            0x0A // R
#define ACC_REG_ADDR_ZOUT_H            0x0B // R
#define ACC_REG_ADDR_DCST_RESP         0x0C // R
#define ACC_REG_ADDR_WHO_AM_I          0x0F // R
#define ACC_REG_ADDR_TILT_POS_CUR      0x10 // R
#define ACC_REG_ADDR_TILT_POS_PRE      0x11 // R

#define ACC_REG_ADDR_INT_SRC_REG1      0x12 // R
#define ACC_REG_ADDR_INT_SRC_REG2      0x13 // R
#define ACC_REG_ADDR_INT_SRC_REG3      0x14 // R
#define ACC_REG_ADDR_STATUS_REG        0x15 // R
#define ACC_REG_ADDR_INT_REL           0x17 // R

#define ACC_REG_ADDR_CTRL_REG1         0x18 // R/W
#define ACC_REG_ADDR_CTRL_REG2         0x19 // R/W
#define ACC_REG_ADDR_CTRL_REG3         0x1A // R/W

#define ACC_REG_ADDR_ODR_CNTL          0x1B // R/W

#define ACC_REG_ADDR_INT_CTRL_REG1     0x1C // R/W
#define ACC_REG_ADDR_INT_CTRL_REG2     0x1D // R/W
#define ACC_REG_ADDR_INT_CTRL_REG3     0x1E // R/W
#define ACC_REG_ADDR_INT_CTRL_REG4     0x1F // R/W
#define ACC_REG_ADDR_INT_CTRL_REG5     0x20 // R/W
#define ACC_REG_ADDR_INT_CTRL_REG6     0x21 // R/W

#define ACC_REG_ADDR_TILT_TIMER        0x22 // R/W
#define ACC_REG_ADDR_WUF_TIMER         0x23 // R/W
#define ACC_REG_ADDR_TDT_EN            0x24 // R/W
#define ACC_REG_ADDR_TDT_TIMER         0x25 // R/W
#define ACC_REG_ADDR_TDT_H_THRESH      0x26 // R/W
#define ACC_REG_ADDR_TDT_L_THRESH      0x27 // R/W
#define ACC_REG_ADDR_TDT_TAP_TIMER     0x28 // R/W
#define ACC_REG_ADDR_TDT_DTAP_TIMER    0x29 // R/W
#define ACC_REG_ADDR_TDT_TAP_TIMER_2   0x2A // R/W
#define ACC_REG_ADDR_TDT_TAP_DTAP_TIMER     0x2B // R/W
#define ACC_REG_ADDR_WUF_THRESH        0x30 // R/W
#define ACC_REG_ADDR_TILT_ANGLE_LL     0x32 // R/W
#define ACC_REG_ADDR_TILT_ANGLE_HL     0x33 // R/W
#define ACC_REG_ADDR_HYST_SET          0x34 // R/W
#define ACC_REG_ADDR_LP_CNTL           0x35 // R/W

#define ACC_REG_ADDR_BUF_CTRL1         0x3A // R/W
#define ACC_REG_ADDR_BUF_CTRL2         0x3B // R/W
#define ACC_REG_ADDR_BUF_STATUS_REG1   0x3C // R
#define ACC_REG_ADDR_BUF_STATUS_REG2   0x3D // R/W
#define ACC_REG_ADDR_BUF_CLEAR         0x3E // W
#define ACC_REG_ADDR_BUF_READ          0x3F // R/W

#define ACC_REG_ADDR_SELF_TEST         0x60 // R/W

// Select register valies
#define REG_VAL_WHO_AM_I               0x14 // (data sheet says 0x14)
#define REG_VAL_START_OK			   0x55

// CTRL1 BIT MASKS
#define ACC_REG_CTRL_PC                0x80 // Power control  '1' On    '0' Off
#define ACC_REG_CTRL_RES               0x40 // Resolution     '1' High  '0' Low
#define ACC_REG_CTRL_DRDYE             0x20 // Data Ready     '1' On    '0' Off
#define ACC_REG_CTRL_GSEL_HI           0x10 // Range     '00' +/-2g    '01' +/-4g
#define ACC_REG_CTRL_GSEL_LO           0x08 //           '10' +/-8g    '11' N/A
#define ACC_REG_CTRL_GSEL_TDTE         0x04 // Directional Tap '1' On   '0' Off
#define ACC_REG_CTRL_GSEL_WUFE         0x02 // Wake Up         '1' On   '0' Off
#define ACC_REG_CTRL_GSEL_TPE          0x01 // Tilt Position   '1' On   '0' Off

uint32_t drv_kxo22_init(void);

#endif   /* _drv_KX022_H_ */

