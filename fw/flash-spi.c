/*  Originally from: https://github.com/im-tomu/foboot/blob/master/sw/src/spi.c
 *  Apache License Version 2.0
 *	Copyright 2019 Sean 'xobs' Cross <sean@xobs.io>
 *  Copyright 2020 Gregory Davill <greg.davill@gmail.com>
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <generated/csr.h>

#include "flash-spi.h"

enum pin {
	PIN_MOSI = 0,
	PIN_CLK = 1,
	PIN_CS = 2,
	PIN_MISO_EN = 3,
	PIN_MISO = 4, // Value is ignored
};

void spiBegin(void) {
	lxspi_bitbang_write((0 << PIN_CLK) | (0 << PIN_CS));
}

void spiEnd(void) {
	lxspi_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS));
}

static void spi_single_tx(uint8_t out) {
	int bit;

	for (bit = 7; bit >= 0; bit--) {
		if (out & (1 << bit)) {
			lxspi_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI));
			lxspi_bitbang_write((1 << PIN_CLK) | (1 << PIN_MOSI));
			lxspi_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI));
		} else {
			lxspi_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI));
			lxspi_bitbang_write((1 << PIN_CLK) | (0 << PIN_MOSI));
			lxspi_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI));
		}
	}
}

static uint8_t spi_single_rx(void) {
	int bit = 0;
	uint8_t in = 0;

	lxspi_bitbang_write((1 << PIN_MISO_EN) | (0 << PIN_CLK));

	while (bit++ < 8) {
		lxspi_bitbang_write((1 << PIN_MISO_EN) | (1 << PIN_CLK));
		in = (in << 1) | lxspi_miso_read();
		lxspi_bitbang_write((1 << PIN_MISO_EN) | (0 << PIN_CLK));
	}

	return in;
}

static uint8_t spi_read_status(void) {
	uint8_t val;

	spiBegin();
	spi_single_tx(0x05);
	val = spi_single_rx();
	spiEnd();
	return val;
}

int spiIsBusy(void) {
  	return spi_read_status() & (1 << 0);
}

void spi_read_uuid(uint8_t* uuid) {
    /* 25Q128JV FLASH, Read Unique ID Number (4Bh) 
        The Read Unique ID Number instruction accesses a factory-set read-only 64-bit number that is unique to
        each W25Q128JV device. The ID number can be used in conjunction with user software methods to help
        prevent copying or cloning of a system. The Read Unique ID instruction is initiated by driving the /CS pin
        low and shifting the instruction code “4Bh” followed by a four bytes of dummy clocks. After which, the 64-
        bit ID is shifted out on the falling edge of CLK
    */

    spiBegin();
    spi_single_tx(0x4B);
    for(int dummy = 0; dummy < 4; dummy++)
        spi_single_rx();
    for(int i = 0; i < 8; i++)
        *uuid++ = spi_single_rx();
    spiEnd();
}

__attribute__((used))
uint32_t spiId(uint8_t* data) {
	uint8_t* ptr = data;

	spiBegin();
	spi_single_tx(0x90);               // Read manufacturer ID
	spi_single_tx(0x00);               // Dummy byte 1
	spi_single_tx(0x00);               // Dummy byte 2
	spi_single_tx(0x00);               // Dummy byte 3
	*ptr++ = spi_single_rx();  // Manufacturer ID
	*ptr++ = spi_single_rx();  // Device ID
	spiEnd();

	spiBegin();
	spi_single_tx(0x9f);               // Read device id
	*ptr++ = spi_single_rx();             // Manufacturer ID (again)
	*ptr++ = spi_single_rx();  // Memory Type
	*ptr++ = spi_single_rx();  // Memory Size
	spiEnd();

	return ptr - data;
}

int spiBeginErase4(uint32_t erase_addr) {
	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(0x20);
	spi_single_tx(erase_addr >> 16);
	spi_single_tx(erase_addr >> 8);
	spi_single_tx(erase_addr >> 0);
	spiEnd();
	return 0;
}

int spiBeginErase32(uint32_t erase_addr) {
	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(0x52);
	spi_single_tx(erase_addr >> 16);
	spi_single_tx(erase_addr >> 8);
	spi_single_tx(erase_addr >> 0);
	spiEnd();
	return 0;
}

int spiBeginErase64(uint32_t erase_addr) {
	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(0xD8);
	spi_single_tx(erase_addr >> 16);
	spi_single_tx(erase_addr >> 8);
	spi_single_tx(erase_addr >> 0);
	spiEnd();
	return 0;
}

int spiBeginWrite(uint32_t addr, const void *v_data, unsigned int count) {
	const uint8_t write_cmd = 0x02;
	const uint8_t *data = v_data;
	unsigned int i;

	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(write_cmd);
	spi_single_tx(addr >> 16);
	spi_single_tx(addr >> 8);
	spi_single_tx(addr >> 0);
	for (i = 0; (i < count) && (i < 256); i++)
		spi_single_tx(*data++);
	spiEnd();

	return 0;
}

uint8_t spiReset(void) {
	// Writing 0xff eight times is equivalent to exiting QPI mode,
	// or if CFM mode is enabled it will terminate CFM and return
	// to idle.
	unsigned int i;
	spiBegin();
	for (i = 0; i < 8; i++)
		spi_single_tx(0xff);
	spiEnd();

	// Some SPI parts require this to wake up
	spiBegin();
	spi_single_tx(0xab);    // Read electronic signature
	spiEnd();

	return 0;
}

int spiInit(void) {

	// Ensure CS is deasserted and the clock is high
	lxspi_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS));

	// Disable memory-mapped mode and enable bit-bang mode
	lxspi_bitbang_en_write(1);

	// Reset the SPI flash, which will return it to SPI mode even
	// if it's in QPI mode, and ensure the chip is accepting commands.
	spiReset();

	return 0;
}

void spiHold(void) {
	spiBegin();
	spi_single_tx(0xb9);
	spiEnd();

}
void spiUnhold(void) {
	spiBegin();
	spi_single_tx(0xab);
	spiEnd();
}

void spiFree(void) {
	// Re-enable memory-mapped mode
	lxspi_bitbang_en_write(0);
}
