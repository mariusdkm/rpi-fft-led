S = /opt/vc/src/hello_pi/hello_fft/hex/shader_256.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_512.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_1k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_2k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_4k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_8k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_16k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_32k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_64k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_128k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_256k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_512k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_1024k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_2048k.hex \
    /opt/vc/src/hello_pi/hello_fft/hex/shader_4096k.hex

C = /opt/vc/src/hello_pi/hello_fft/mailbox.c /opt/vc/src/hello_pi/hello_fft/gpu_fft.c /opt/vc/src/hello_pi/hello_fft/gpu_fft_base.c /opt/vc/src/hello_pi/hello_fft/gpu_fft_twiddles.c /opt/vc/src/hello_pi/hello_fft/gpu_fft_shaders.c
C1D = $(C) rgb_spectrum.c
C2D = $(C) fft_multithreading.c spi_led.c led_modes.c

H = /opt/vc/src/hello_pi/hello_fft/gpu_fft.h /opt/vc/src/hello_pi/hello_fft/mailbox.h spi_led.h led_modes.h

F = -lrt -lm -ldl -lsndfile -lasound -lwiringPi -pthread
#F2D = -lrt -lm -ldl -lasound

#rgb_spectrum.bin:	$(S) $(C1D) $(H)
#	gcc -o rgb_spectrum.bin $(F) $(C1D)

fft.bin:	$(S) $(C2D) $(H)
	gcc -o fft.bin $(F) $(C2D)

clean:
	rm -f *.bin
