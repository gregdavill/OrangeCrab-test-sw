/* This file is part of OrangeCrab-test
 *
 * Copyright 2020 Gregory Davill <greg.davill@gmail.com> 
 */

#include <stdlib.h>
#include <stdio.h>

#include <generated/csr.h>
#include <generated/mem.h>
#include <generated/git.h>

#include <irq.h>
#include <uart.h>

#include <sdram.h>

#include <sleep.h>
#include <flash-spi.h>


void print_buffer(uint8_t* ptr, uint8_t len){
	for(int i = 0; i < len; i++){
		printf("%s0x%02x\"",i > 0 ? ",\"" : "\"", ptr[i]);
	}
}

int main(int i, char **c)
{	
	/* Setup IRQ, needed for UART */
	irq_setmask(0);
	irq_setie(1);
	uart_init();

	
	printf("\n");

	printf("Hello from OrangeCrab! o/ \n");
 	printf("{\"firmware build date\":\""__DATE__ "\", \"firmware build time\": \"" __TIME__ "\"}\n");

 	printf("{\"migen sha1\":\""MIGEN_GIT_SHA1"\"}\n");
 	printf("{\"litex sha1\":\""LITEX_GIT_SHA1"\"}\n");

	
	//msleep(100);

	/* Init Memory */
	int sdr_ok = sdrinit();
	
	while(!sdr_ok){

	}

	self_reset_out_write(0xAA550001);

	/* Check for SPI FLASH */	
	spiInit();
	unsigned char buf[8] = {0};
	spiId(buf);

	printf("{\"spi id\":[");
	print_buffer(buf, 5);
	printf("]}\n");

	// Check Flash UUID
	spi_read_uuid(buf);
	printf("{\"spi uuid\":[");
	print_buffer(buf, 8);
	printf("]}\n");


	/* Test of LED GPIO */
	uint8_t led_gpio_patterns[] = {0x0, 0x1, 0x2, 0x4, 0x7};
	for(int i = 0; i < sizeof(led_gpio_patterns); i++){
		uint8_t out_pattern = led_gpio_patterns[i];
		gpio_led_out_write(0);
		gpio_led_oe_write(out_pattern);

		msleep(1);

		uint8_t read_pattern = gpio_led_in_read();

		/* Print values */
		printf("{%01X:%01X}\n",out_pattern, read_pattern);
		
	}

	/* Quick test of GPIO */
	for(int i = 0; i < 13; i++){
		uint32_t out_pattern = 1 << i;
		gpio_out_write(0xFFFFFFFF);
		gpio_oe_write(out_pattern);

		msleep(1);

		uint32_t read_pattern = gpio_in_read();

		/* Print values */
		printf("LOW{%04X:%04X}\n",out_pattern, read_pattern);
		
		gpio_out_write(0);

		msleep(1);

		read_pattern = gpio_in_read();

		/* Print values */
		printf("HIGH{%04X:%04X}\n",out_pattern, read_pattern);
		
	}


	return 0;
}
