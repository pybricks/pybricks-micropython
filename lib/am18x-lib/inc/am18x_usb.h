// tary, 21:10 2015/6/4

#ifndef __AM18X_USB_H__
#define __AM18X_USB_H__

#include "am18x_map.h"

typedef struct {
	am18x_bool highspeed;
	am18x_bool host_n_device;
} usb0_conf_t;

am18x_rt usb0_conf(const usb0_conf_t* conf);
uint32_t usb0_intr_state(void);
am18x_rt usb0_intr_clear(void);

#endif//__AM18X_USB_H__
