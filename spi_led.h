#ifndef spi_led_h
#define spi_led_h

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// number of physical leds in the strip
#define num_leds 28


// declare the struct with the three colors
struct led_data {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
// forward declare led_data to be a struct
typedef struct led_data led_data;
//aborts programm with a message
void pabort(const char *s);

//Has to be called at the begin of the Programm returns fd
int init_spi_led();

//Write the FFT Buffer and display it
void write_spi_buffer(led_data ledstrip_data[num_leds], int fd);

int close_spi_led(int fd);

#endif /* spi_led_h */
