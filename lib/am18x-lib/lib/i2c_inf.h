// tary, 1:42 2013/4/28
#ifndef __I2C_INF_H__
#define __I2C_INF_H__

int i2c_init(I2C_con_t* bus, uint32_t freq);
int i2c_read(I2C_con_t* bus, uint16_t dev, uint8_t* bytes, uint32_t cnt);
int i2c_write(I2C_con_t* bus, uint16_t dev, cuint8_t* bytes, uint32_t cnt);

#endif//__I2C_INF_H__
