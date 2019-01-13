# rpi-fft-light
This is a Raspberry Pi Projekt for the German Jugend Forscht competition.
This Code controlls WS2812B LEDs using the SPI port of the Raspberry Pi by calculating a Fast Fourier Transform.
The code I wrote makes use of the GPU_FFT library (http://www.aholme.co.uk/GPU_FFT/Main.htm) and 
code to steer the WS2812b LEDs (https://github.com/penfold42/stuff/blob/master/ws2812_spi.c).

fft.c is the main file and requires 1 or 2 arguments, the
1st is a mono .wav file to play, and the
2nd is a boolean controlling if the frequency spectrum of each time frame should be scaled to 1 for the highest FFT amplitude.

Run the code using sudo since the GPU_FFT code makes use of the GPU, which requires root permissions. 

The file led_modes.c specifies the different modes I implemented so far that determine how to translate the FFT to the LEDs.

In developing this, I made use of the following information:
http://www.aholme.co.uk/GPU_FFT/Main.htm
https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum
https://github.com/penfold42/stuff/blob/master/ws2812_spi.c
https://www.linuxjournal.com/article/6735
https://ubuntuforums.org/showthread.php?t=968690

Copyright (c) 2018 mariusdkm
for the spi interface Copyright (c) 2016 penfold42
for the GPU_FFT Copyright (c) 2014 Andrew Holme.

This program is free software; you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by
the Free Softare Foundation, version 2.
