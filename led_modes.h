//
//  led_modes.h
//  
//
//  Created by Marius Dario Cora on 05.01.19.
//

#ifndef led_modes_h
#define led_modes_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "spi_led.h"

void mode1(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]);
void mode2(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]);
void mode3(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]);
void mode4(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]);

#endif /* led_modes_h */
