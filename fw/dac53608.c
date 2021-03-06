// Copyright 2020 Gregory Davill <greg.davill@gmail.com> 
// Copyright 2020 Michael Welling <mwelling@ieee.org>
#include <dac53608.h>
#include <i2c.h>

#define DAC_I2C_ADDR 0b1001000

static bool dac_write(uint8_t addr, uint16_t d)
{
	char data[2];
	data[0] = d >> 8;
	data[1] = d & 0xFF;

	return i2c_write(DAC_I2C_ADDR, addr, data, 2);
}

static bool dac_read(uint8_t addr, uint16_t* d)
{
	char data[2] = {0};

	bool ret = i2c_read(DAC_I2C_ADDR, addr, data, 2, false);
	*d = data[0] << 8 | data[1];

	return ret;
}

bool dac_write_channel(uint8_t index, uint16_t val)
{
	return dac_write(index+8, val);
}

void dac_reset()
{

	i2c_reset();

	msleep(1);

	//printf("Write i2c: Reset. %u\n", ret);
	bool ret = dac_write(2, 0x000A);
	/* Wait for it to reset */
	msleep(100);

	/* enable all DAC channels */
	dac_write(1, 0x0000);

}

bool dac_read_id()
{

	uint16_t value = 0;
	bool ret = dac_read(2, &value);

	if(ret == true & value == 0x0300)
		return true;
	return false;
}
