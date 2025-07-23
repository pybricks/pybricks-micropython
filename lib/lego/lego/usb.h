// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

// LEGO USB stuff
// https://github.com/pybricks/technical-info/blob/master/assigned-numbers.md#usb


#ifndef _LEGO_USB_H_
#define _LEGO_USB_H_

/** Official LEGO USB Vendor ID. */
#define LEGO_USB_VID 0x0694
/** Official LEGO USB Product ID for MINDSTORMS NXT. */
#define LEGO_USB_PID_NXT 0x0002
/** Official LEGO USB Product ID for MINDSTORMS EV3. */
#define LEGO_USB_PID_EV3 0x0005
/** Official LEGO USB Product ID for MINDSTORMS EV3. */
#define LEGO_USB_PID_EV3_UPDATE 0x0006
/** Official LEGO USB Product ID for SPIKE Prime in DFU mode. */
#define LEGO_USB_PID_SPIKE_PRIME_DFU 0x0008
/** Official LEGO USB Product ID for SPIKE Prime. */
#define LEGO_USB_PID_SPIKE_PRIME 0x0009
/** Official LEGO USB Product ID for SPIKE Essential in DFU mode. */
#define LEGO_USB_PID_SPIKE_ESSENTIAL_DFU 0x000C
/** Official LEGO USB Product ID for SPIKE Essential. */
#define LEGO_USB_PID_SPIKE_ESSENTIAL 0x000D
/** Official LEGO USB Product ID for MINDSTORMS Robot Inventor. */
#define LEGO_USB_PID_ROBOT_INVENTOR 0x0010
/** Official LEGO USB Product ID for MINDSTORMS Robot Inventor in DFU mode. */
#define LEGO_USB_PID_ROBOT_INVENTOR_DFU 0x0011

/** Official LEGO USB Manufacturer String. */
#define LEGO_USB_MFG_STR u"LEGO System A/S"
/** NXT does not officially come with a product string */
#define LEGO_USB_PROD_STR_NXT u"NXT"
/** Official LEGO USB Product String for MINDSTORMS EV3. */
#define LEGO_USB_PROD_STR_EV3 u"LEGO MINDSTORMS EV3"
/** Official LEGO USB Product String for SPIKE Prime and MINDSTORMS Robot Inventor. */
#define LEGO_USB_PROD_STR_TECHNIC_LARGE_HUB u"LEGO Technic Large Hub"
/** Official LEGO USB Product String for SPIKE Essential. */
#define LEGO_USB_PROD_STR_TECHNIC_SMALL_HUB u"LEGO Technic Small Hub"

#endif // _LEGO_USB_H_
