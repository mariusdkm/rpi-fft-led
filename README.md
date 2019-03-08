# rpi-fft-led
This is a Raspberry Pi Projekt for the German Jugend Forscht competition.
This Code controlls WS2812B LEDs using the SPI port of the Raspberry Pi by calculating a Fast Fourier Transform.
The code I wrote makes use of the GPU_FFT library (http://www.aholme.co.uk/GPU_FFT/Main.htm) and 
code to steer the WS2812b LEDs (https://github.com/penfold42/stuff/blob/master/ws2812_spi.c).

[fft.c](fft.c) is the main file and requires 1 to 3 arguments, the
- 1st is a mono .wav file to play **or** "mic" if you want to use an external microphone
- 2nd is the mode 1-4 to for displaying the sound
- 3nd is optional. If set to "log" displays Bins logarythmically on LED-Strip. If set as something else the frequency spectrum of each time frame should be scaled to 1 for the highest FFT amplitude.

The Modes are in the [led_modes.c](led_modes.c) file
 * **Mode 1** maps each LED a number of bins and displays a color spectrum and the brightness is the amplitude(has to be optimized).
 * **Mode 2** maps each LED a number of bins and the color is the amplitude.
 * **Mode 3** is the same as mode 2 but the base is additional brightness.
 * **Mode 4** displays a snake that always adds the frequency with the highest amplitude to the beginning.


Run the code using sudo since the GPU_FFT code makes use of the GPU, which requires root permissions.

An example call would be:
```bash
sudo ./fft.bin example.wav 4
```
or
```bash
sudo ./fft.bin mic 2
```
The [Langfassung.pdf](Langfassung.pdf) is the Document I wrote for the Jugend Forscht Contest.
It is in German but goes a bit deeper into how and why I've built this Project.


In developing this, I made use of the following information:
http://www.aholme.co.uk/GPU_FFT/Main.htm
https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum
https://github.com/penfold42/stuff/blob/master/ws2812_spi.c
https://www.linuxjournal.com/article/6735
https://ubuntuforums.org/showthread.php?t=968690


For the spi interface Copyright (c) 2016 penfold42

For the GPU_FFT Copyright (c) 2014 Andrew Holme.

Copyright (c) 2018 mariusdkm

This program is free software; you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by
the Free Softare Foundation, version 2.
