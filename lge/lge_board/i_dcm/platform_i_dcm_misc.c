/*
  * Copyright (C) 2009 LGE, Inc.
  * 
  * Author: Sungwoo Cho <sungwoo.cho@lge.com>
  *
  * This software is licensed under the terms of the GNU General Public
  * License version 2, as published by the Free Software Foundation, and
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  */

#include <linux/types.h>
#include <linux/list.h>
#include <linux/err.h>
#include <mach/msm_iomap.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <board_lge.h>

#include "devices_i_dcm.h"

#ifdef CONFIG_LGE_ISA1200
#include <linux/i2c.h>
#include "lge_isa1200.h"
#endif

#define REG_WRITEL(value, reg)	        writel(value, (MSM_CLK_CTL_BASE+reg))
#define REG_READL(reg)	        readl((MSM_CLK_CTL_BASE+reg))

#define VIBE_IC_VOLTAGE		        3000000
#define GPn_MD_REG(n)		                (0x2D00+32*(n))
#define GPn_NS_REG(n)		                (0x2D24+32*(n))
#define GP1_MD_REG                                 GPn_MD_REG(1)
#define GP1_NS_REG                                  GPn_NS_REG(1)

#ifdef CONFIG_ANDROID_VIBRATOR
#define GP1_M_DEFAULT			        127
#define GP1_N_DEFAULT			        255
#define GP1_D_DEFAULT		            	(GP1_N_DEFAULT >> 1) 
#define PWM_MAX_HALF_DUTY		((GP1_N_DEFAULT >> 1) - 8) /* 127 - 8 = 119  minimum operating spec. should be checked */

static struct regulator *snddev_reg_l1 = NULL;

static int vibrator_power_set(int enable)
{
	int rc;

	printk(KERN_INFO "LGE: %s  enable=%d\n", __func__, enable);

	if (enable) {

		if (NULL == snddev_reg_l1) {

			snddev_reg_l1 = regulator_get(NULL, "8901_l1");

			if (IS_ERR(snddev_reg_l1)) {
				pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
				"l1", PTR_ERR(snddev_reg_l1));
				return -EBUSY;
			}
		
			rc = regulator_set_voltage(snddev_reg_l1, VIBE_IC_VOLTAGE, VIBE_IC_VOLTAGE);
	
			if (rc < 0)
				pr_err("%s: regulator_set_voltage(l1) failed (%d)\n",
			__func__, rc);

			rc = regulator_enable(snddev_reg_l1);

			if (rc < 0)
				pr_err("%s: regulator_enable(l1) failed (%d)\n", __func__, rc);
		}

	} else {
/*		if (regulator_is_enabled(snddev_reg_l1)) { */
		if (snddev_reg_l1) {
			rc = regulator_disable(snddev_reg_l1);

			if (rc < 0)
				pr_err("%s: regulator_enable(l1) failed (%d)\n", __func__, rc);

			regulator_put(snddev_reg_l1);
			snddev_reg_l1 = NULL;
		}
	}

/*	regulator_put(snddev_reg_l1); */

	return 0;
}

static int vibrator_pwm_set(int enable, int amp)
{

	uint M_VAL = GP1_M_DEFAULT;
	uint N_VAL = GP1_N_DEFAULT;
	uint D_VAL = GP1_D_DEFAULT;

	printk(KERN_INFO "LGE: %s amp=%d\n", __func__, amp);

	if (enable) {

//                   D_VAL = ((PWM_MAX_HALF_DUTY*amp) >> 7)+ GP1_D_DEFAULT;
                    D_VAL = amp;
                    REG_WRITEL( 
                    (((M_VAL & 0xffU) <<16U) + /* M_VAL[23:16] */
                    ((~(D_VAL<<1)) & 0xffU)),  /* D_VAL[7:0] */
                    GP1_MD_REG);
                    REG_WRITEL( 
                    ((((~(N_VAL-M_VAL))& 0xffU) <<16U) + /* N_VAL[23:16] */
                    (1U<<11U) +  /* CLK_ROOT_ENA[11]  : Enable(1) */
                    (0U<<10U) +  /* CLK_INV[10]       : Disable(0) */
                    (1U<<9U) +	 /* CLK_BRANCH_ENA[9] : Enable(1) */
                    (1U<<8U) +   /* NMCNTR_EN[8]      : Enable(1) */
                    (0U<<7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
                    (2U<<5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
                    (3U<<3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
                    (0U<<0U)),   /* SRC_SEL[2:0]      : pxo (0)  */
                    GP1_NS_REG);
                    printk("[LGE:vibrator] GPIO_LIN_MOTOR_PWM is enable with M=%d N=%d D=%d\n", M_VAL, N_VAL, D_VAL);
	} else {	
                    REG_WRITEL( 
                    (((M_VAL & 0xffU) <<16U) + /* M_VAL[23:16] */
                    ((~(D_VAL<<1)) & 0xffU)),  /* D_VAL[7:0] */
                    GP1_MD_REG);
                    REG_WRITEL( 
                    ((((~(N_VAL-M_VAL))& 0xffU) <<16U) + /* N_VAL[23:16] */
                    (0U<<11U) +  /* CLK_ROOT_ENA[11]  : Disable(0) */
                    (0U<<10U) +  /* CLK_INV[10] 	  : Disable(0) */
                    (0U<<9U) +	 /* CLK_BRANCH_ENA[9] : Disable(0) */
                    (0U<<8U) +   /* NMCNTR_EN[8]      : Disable(0) */
                    (0U<<7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
                    (2U<<5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
                    (3U<<3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
                    (0U<<0U)),   /* SRC_SEL[2:0]      : pxo (0)  */
                    GP1_NS_REG);
                    printk("[LGE:vibrator] GPIO_LIN_MOTOR_PWM is disalbe \n");
	}
	return 0;
}

static int vibrator_ic_enable_set(int enable)
{
	printk(KERN_INFO "LGE: %s  enable=%d\n", __func__, enable);

	if (enable) {
		gpio_set_value_cansleep(GPIO_LIN_MOTOR_EN, 1);
                  printk("[LGE:vibrator] GPIO_LIN_MOTOR_EN gpio value :%d  \n", gpio_get_value(GPIO_LIN_MOTOR_EN));
	} else {	
		gpio_set_value_cansleep(GPIO_LIN_MOTOR_EN, 0);
                printk("[LGE:vibrator] GPIO_LIN_MOTOR_EN gpio value :%d  \n", gpio_get_value(GPIO_LIN_MOTOR_EN));

	}
	return 0;
}

static int vibrator_init(void)
{
    int rc;

    snddev_reg_l1 = NULL;

    /* GPIO setting for Motor EN*/
    rc = gpio_request(GPIO_LIN_MOTOR_EN, "lin_motor_en");
    if (rc) {
        pr_err("%s: GPIO_LIN_MOTOR_EN %d request failed\n",
            __func__, GPIO_LIN_MOTOR_EN);
        return 0;
    }

//    gpio_direction_output(GPIO_LIN_MOTOR_EN, 0);

//    gpio_free(GPIO_LIN_MOTOR_EN);

    printk("[LGE:vibrator] GPIO_LIN_MOTOR_EN gpio value :%d  \n", gpio_get_value(GPIO_LIN_MOTOR_EN));

	vibrator_ic_enable_set(0);
	vibrator_pwm_set(0,0);
//	vibrator_power_set(0);

#if 0
    /* GPIO setting for Motor PWM */
    rc = gpio_request(GPIO_LIN_MOTOR_PWM, "lin_motor_pwn");
    if (rc) {
            pr_err("%s: GPIO_LIN_MOTOR_PWM %d request failed\n",
                __func__, GPIO_LIN_MOTOR_PWM);
            return 0;
    }

    gpio_direction_output(GPIO_LIN_MOTOR_PWM, 1);
    gpio_free(GPIO_LIN_MOTOR_PWM);
    printk("[LGE:vibrator] GPIO_LIN_MOTOR_PWM  \n");
#endif

    return 0;
}

static struct android_vibrator_platform_data vibrator_data = {
	.enable_status = 0,
	.power_set = vibrator_power_set,
	.pwm_set = vibrator_pwm_set,
	.ic_enable_set = vibrator_ic_enable_set,
	.vibrator_init= vibrator_init,
};

static struct platform_device android_vibrator_device = {
	.name   = "android-vibrator",
	.id = -1,
	.dev = {
		.platform_data = &vibrator_data,
	},
};

static struct platform_device *misc_devices[] __initdata = {
	&android_vibrator_device,
};
#endif

#ifdef CONFIG_LGE_ISA1200

static int lge_isa1200_clock(int enable)
{
	if (enable) {
                    REG_WRITEL( 
                    ((( 0 & 0xffU) <<16U) + /* N_VAL[23:16] */
                    (1U<<11U) +  /* CLK_ROOT_ENA[11]  : Enable(1) */
                    (0U<<10U) +  /* CLK_INV[10]       : Disable(0) */
                    (1U<<9U) +	 /* CLK_BRANCH_ENA[9] : Enable(1) */
                    (0U<<8U) +   /* NMCNTR_EN[8]      : Enable(1) */
                    (0U<<7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
                    (0U<<5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
                    (0U<<3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
                    (0U<<0U)),   /* SRC_SEL[2:0]      : pxo (0)  */
                    GP1_NS_REG);
                    //printk("GPIO_LIN_MOTOR_PWM is enabled. pxo clock.");
	} else {	
                    REG_WRITEL( 
                    ((( 0 & 0xffU) <<16U) + /* N_VAL[23:16] */
                    (0U<<11U) +  /* CLK_ROOT_ENA[11]  : Disable(0) */
                    (0U<<10U) +  /* CLK_INV[10] 	  : Disable(0) */
                    (0U<<9U) +	 /* CLK_BRANCH_ENA[9] : Disable(0) */
                    (0U<<8U) +   /* NMCNTR_EN[8]      : Disable(0) */
                    (1U<<7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
                    (0U<<5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
                    (0U<<3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
                    (0U<<0U)),   /* SRC_SEL[2:0]      : pxo (0)  */
                    GP1_NS_REG);
                    //printk("GPIO_LIN_MOTOR_PWM is disabled.");
	}
	return 0;
}

static struct isa1200_reg_cmd isa1200_init_seq[] = {

	{LGE_ISA1200_HCTRL2, 0x00},		/* bk : release sw reset */

	{LGE_ISA1200_SCTRL , 0x0F}, 		/* LDO:3V */

	{LGE_ISA1200_HCTRL0, 0x10},		/* [4:3]10 : PWM Generation Mode [1:0]01 : Divider 1/256 */
	{LGE_ISA1200_HCTRL1, 0xC0},		/* [7] 1 : Ext. Clock Selection, [5] 0:LRA, 1:ERM */
	{LGE_ISA1200_HCTRL3, 0x33},		/* [6:4] 1:PWM/SE Generation PLL Clock Divider */

	{LGE_ISA1200_HCTRL4, 0x81},		/* bk */
	{LGE_ISA1200_HCTRL5, 0x3a},		/* [7:0] PWM High Duty(PWM Gen) 0-6B-D6 */ /* TODO make it platform data? */
	{LGE_ISA1200_HCTRL6, 0x74},		/* [7:0] PWM Period(PWM Gen) */ /* TODO make it platform data? */

};

static struct isa1200_reg_seq isa1200_init = {
   .reg_cmd_list = isa1200_init_seq,
   .number_of_reg_cmd_list = ARRAY_SIZE(isa1200_init_seq),
};

static struct lge_isa1200_platform_data lge_isa1200_platform_data = {
	.vibrator_name = "vibrator",

	.gpio_hen = GPIO_LIN_MOTOR_EN,
	.gpio_len = GPIO_LIN_MOTOR_EN,

	.clock = lge_isa1200_clock,

	.max_timeout = 30000,

	.default_vib_strength = 213,

	.init_seq = &isa1200_init,
};

#define ISA1200_SLAVE_ADDR 0x90

static struct i2c_board_info lge_i2c_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("lge_isa1200", ISA1200_SLAVE_ADDR>>1),
//		I2C_BOARD_INFO("vibrator_i2c", ISA1200_SLAVE_ADDR>>1), /* TODO */
		.platform_data = &lge_isa1200_platform_data
	},
};

#endif

void __init lge_add_misc_devices(void)
{
	printk(KERN_INFO "LGE: %s \n", __func__);

#ifdef CONFIG_ANDROID_VIBRATOR
   if(lge_bd_rev <= LGE_REV_A)
	platform_add_devices(misc_devices, ARRAY_SIZE(misc_devices));
#endif

#ifdef CONFIG_LGE_ISA1200
	/* isa1200 initialization code */
	if(lge_bd_rev > LGE_REV_A)
	i2c_register_board_info(LGE_GSBI_BUS_ID_VIB_ISA1200,
		lge_i2c_isa1200_board_info,
		ARRAY_SIZE(lge_i2c_isa1200_board_info));
#endif
}

