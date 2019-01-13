S = hex/shader_256.hex \
    hex/shader_512.hex \
    hex/shader_1k.hex \
    hex/shader_2k.hex \
    hex/shader_4k.hex \
    hex/shader_8k.hex \
    hex/shader_16k.hex \
    hex/shader_32k.hex \
    hex/shader_64k.hex \
    hex/shader_128k.hex \
    hex/shader_256k.hex \
    hex/shader_512k.hex \
    hex/shader_1024k.hex \
    hex/shader_2048k.hex \
    hex/shader_4096k.hex

C = mailbox.c gpu_fft.c gpu_fft_base.c gpu_fft_twiddles.c gpu_fft_shaders.c
C1D = $(C) rgb_spectrum.c
C2D = $(C) fft.c spi_led.c led_modes.c

H = gpu_fft.h mailbox.h spi_led.h led_modes.h

F = -lrt -lm -ldl -lsndfile -lasound
#F2D = -lrt -lm -ldl -lasound

#rgb_spectrum.bin:	$(S) $(C1D) $(H)
#	gcc -o rgb_spectrum.bin $(F) $(C1D)

fft.bin:	$(S) $(C2D) $(H)
	gcc -o fft.bin $(F) $(C2D)

clean:
	rm -f *.bin
