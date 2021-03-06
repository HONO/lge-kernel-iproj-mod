/*
 *  felicagpio.h
 *  
 */

#ifndef __FELICA_GPIO_H__
#define __FELICA_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  INCLUDE FILES FOR MODULE
 */

#include <linux/list.h>

#include "felica_common.h"
/*
 *  DEFINE
 */

/* common */ 
enum{
  GPIO_DIRECTION_IN = 0,
  GPIO_DIRECTION_OUT,
};

enum{
  GPIO_LOW_VALUE = 0,
  GPIO_HIGH_VALUE,
};

enum{
  GPIO_CONFIG_DISABLE = 0,
  GPIO_CONFIG_ENABLE,
};

/* felica_pon */
#define GPIO_FELICA_PON   107

/* felica_rfs */
#define GPIO_FELICA_RFS   128

/* felica_int */
#define GPIO_FELICA_INT   125

/* felica_lockcont */
#define GPIO_FELICA_LOCKCONT   123  

/*
 *  FUNCTION PROTOTYPE
 */
int felica_gpio_open(int gpionum, int direction, int value);
void felica_gpio_write(int gpionum, int value);
int felica_gpio_read(int gpionum);

#ifdef __cplusplus
}
#endif

#endif // __FELICA_RFS_H__
