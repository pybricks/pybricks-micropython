// tary, 13:11 2012/12/28

#ifndef __AM18X_SYSCFG_H__
#define __AM18X_SYSCFG_H__

#include "am18x_map.h"

am18x_rt syscfg_kick(am18x_bool lock);
am18x_rt syscfg_pll(am18x_bool lock);
am18x_rt syscfg_aync3(am18x_bool src_y_pll0_n_pll1);
// pos = [0,4,8,12,16,20,24,28]
// val = [0..15]
am18x_rt syscfg_pinmux(uint32_t mux, uint32_t pos, uint32_t val);
int32_t syscfg_bootmode(void);
am18x_rt syscfg_vtpio_calibrate(void);
am18x_rt syscfg_ddr_slew(am18x_bool ddr2_not_mddr);
am18x_rt syscfg_set_usb0phy(am18x_bool y_on_n_off, uint32_t freq);
am18x_rt syscfg_set_usb1phy(am18x_bool y_on_n_off, am18x_bool phy_clk_y_usb0_n_refin);

#endif//__AM18X_SYSCFG_H__
