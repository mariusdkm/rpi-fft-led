# rpi-fft-light
This is a Raspberry Pi Projekt for the German Jugend Forscht Competition
This Code controlls WS2812B LEDs with the SPI port of the Raspberry Pi by calculating a FFT with the GPU_FFT library.
I don't think I have copyright.


fft.c is the main File and gets 1 or 2 arguments
1st is the .WAV mono file to play 
2nd is if it should optimize so scale the audio up to an amplitude of 1

spi_led.c is the file for driving the LEDs

led_modes.c are the modes how to translate the FFT to the LEDs
Feel free to try around

sources: 
http://www.aholme.co.uk/GPU_FFT/Main.htm
https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum
https://github.com/penfold42/stuff/blob/master/ws2812_spi.c
https://www.linuxjournal.com/article/6735
https://ubuntuforums.org/showthread.php?t=968690
