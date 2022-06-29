/* WS2812b LED strip example (ab)using the SPI device
 * based on:
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2016  penfold42 on github
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 *
 * Timings from: https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
 * Symbol 	Parameter 		Min 	Typical 	Max 	Units
 * T0H 	0 code ,high voltage time       200 	350 	500 	nS
 * T1H 	1 code ,high voltage time       550 	700 		nS
 * TLD 	data, low voltage time          450 	600 	5,000 	nS
 * TLL 	latch, low voltage time        6,000 			ns
 *
 * If the SPI bus is running fast enough, you can (ab)use it to send the correctly timed
 * pulses on the MOSI pin.
 *
 * If you target a bit period of 350nS, a SPI clock of 2,857,142 MHz _should_ work
 * - sending bit pattern 1000 => T0H 350nS, T0L 1050 nS
 * - sending bit pattern 1100 => T1H 700nS, T1L  700 nS
 * this meets the timing criteria and each pair of LED bits fits neatly in a single SPI byte
 *
 * The latch period of 6,000 nS requires 18 SPI bits of '0' (6000/350)
 * It's simpler just to send 3 bytes of 0.
 *
 * As of 12/5/2016 I've found that I really need a speed 3,800,000
 * This is because my PI was undervolt and it was automatically dropping core clock speed
 
 * If the speed is too low, the strip will be white or flickering white
 * If the speed is too high, the strip will be black or flickering black
 *
 */

#include "spi_led.h"

// forward declare led_data to be a struct


// a structure encoding the rgb value for all of the leds in the strip
//led_data ledstrip_data[num_leds];


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 4000000;    // = 250nS
static uint16_t delay;

// each pair of WS2812 bits is sent as 1 spi byte
// looks up 2 bits of led data to get the spi pattern to send
// A ws2812 '0' is sent as 1000
// A ws2812 '1' is sent as 1100
// A ws2812 'reset/latch' is sent as 3 bytes of 00000000
uint8_t bitpair_to_byte[] = {
	0b10001000,
	0b10001100,
	0b11001000,
	0b11001100,
};

uint8_t nled_data[] = {
	0x55, 0xaa, 0x55,
};

// each led needs 3 bytes
// each led byte needs 4 SPI bytes
// +3 for the reset/latch pulse
uint8_t tx[num_leds*3*4+3];

static int write_led_byte(int spi_ptr,uint8_t led_color)
{
	tx[spi_ptr++] = bitpair_to_byte[(led_color&0b11000000)>>6];
	tx[spi_ptr++] = bitpair_to_byte[(led_color&0b00110000)>>4];
	tx[spi_ptr++] = bitpair_to_byte[(led_color&0b00001100)>>2];
	tx[spi_ptr++] = bitpair_to_byte[(led_color&0b00000011)>>0];
	return spi_ptr;
}

// Curiously the order we need to write the bytes to the spi is green, red, blue !
static void make_spi_buffer(led_data ledstrip_data[num_leds])
{
	unsigned spi_ptr=0;
	for (unsigned led_nr=0; led_nr<num_leds; led_nr++ ) {
		spi_ptr = write_led_byte(spi_ptr,ledstrip_data[led_nr].g);
		spi_ptr = write_led_byte(spi_ptr,ledstrip_data[led_nr].r);
		spi_ptr = write_led_byte(spi_ptr,ledstrip_data[led_nr].b);
	}
}



static void transfer(int fd)
{
	int ret;
	
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = 0,
		.len = ARRAY_SIZE(tx),
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
}

void write_spi_buffer(led_data ledstrip_data[num_leds], int fd)
{
	make_spi_buffer(ledstrip_data);
	transfer(fd);
	
}


int init_spi_led(uint32_t spi_speed){
	int fd, ret;
	fd = open(device, O_RDWR);
	speed = spi_speed;
	if (fd < 0)
		pabort("can't open device");
	
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");
	
	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");
	
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");
	
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");
	
	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");
	
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");
	
	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	
	return fd;
}

int close_spi_led(int fd){
	return close(fd);
}
