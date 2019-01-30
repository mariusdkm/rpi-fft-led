#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>

// for reading wav files:
#include <sndfile.h>

//GPU_FFT librarys
#include "mailbox.h"
#include "gpu_fft.h"

#include "spi_led.h"
#include "led_modes.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
//ALSA library
#include <alsa/asoundlib.h>


int main(int argc, char *argv[]) {
	//ALSA audio device init-------------------
	int err;
	int16_t *buffer;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	
	/* Open PCM device for playback. */
	rc = snd_pcm_open(&handle, "default",
					  SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr,
				"unable to open pcm device: %s\n",
				snd_strerror(rc));
		exit(1);
	}
	
	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);
	
	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);
	
	/* Set the desired hardware parameters. */
	
	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,SND_PCM_ACCESS_RW_INTERLEAVED);
	
	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params, format);
	
	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 1);
	
	/* 44100 bits/second sampling rate (CD quality) */
	val = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
	
	/* Set period size to 32 frames. */
	frames = 1024;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
	
	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr,
				"unable to set hw parameters: %s\n",
				snd_strerror(rc));
		exit(1);
	}
	
	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames,
									  &dir);
	
	//allocate buffer of 16bit ints, as specified in PCM_FORMAT
	buffer = malloc(frames * snd_pcm_format_width(format) / 8 * 2);
	//File reading code-----------------------
	SNDFILE *sf;
	SF_INFO sf_info;
	int num_channels, num, num_items;
	short *buf;
	FILE *out;
	bool optimize = false;
	
	int num_bins,current_bin,current_led;
	float max,avg,amplitude,max_amplitude = 0,amplitude_factor,freq_scaling;
	uint32_t spi_speed = 4000000; //=250ns
	
	if (argc > 2)
	{
		optimize = true;
		printf("optimizing enabled\n");
	}
	
	 //Open the WAV file.
	 if (argc < 2 || argc > 3)
		 pabort("Call me with the name of the sound file as argument (and optionally a second argument to optimize display for this sound file)");
	
	 
	 sf_info.format = 0;
	 sf = sf_open(argv[1],SFM_READ,&sf_info);
	 if (sf == NULL)
	 {
	 printf("Failed to open the file.\n");
	 exit(-1);
	 }
	 //Print some of the sf_info, and figure out how much data to read.
	 printf("frames=%d\n",sf_info.frames);
	 printf("samplerate=%d\n",sf_info.samplerate);
	 printf("channels=%d\n",sf_info.channels);
	 printf("format=%d\n",sf_info.format);
	 printf("sections=%d\n",sf_info.sections);
	 printf("seekable=%d\n",sf_info.seekable);
	 
	 num_items = sf_info.frames*sf_info.channels;
	 printf("num_items=%d\n",num_items);
	 //Allocate space for the data to be read, then read it.
	 buf = (short *) malloc(num_items*sizeof(short));
	 num = sf_read_short(sf,buf,num_items);
	 sf_close(sf);
	 printf("Read %d items\n",num);
	 
	 // code below only works for mono files, so abort here if not mono
	 if (sf_info.channels > 1)
	 pabort("Please use a mono wav file, not stereo.");
	// -------------------------------------------------------------------
	
	
	int ret = 0;
	int fd, i;
	int color;
	
	//FFT Variables
	float * OutData;
	int jobs=1, c, mb = mbox_open(), NLOG2 = 10;
	double r,g,b;
	
	struct GPU_FFT_COMPLEX * base;
	struct GPU_FFT * fft;
	struct GPU_FFT_COMPLEX *DataIn, *DataOut;
	unsigned long N = 1 << NLOG2;
	float FloatN = (float) N;
	float HalfN  = FloatN / 2.0;
	unsigned int rate = 44100;
	
	// a structure encoding the rgb value for all of the leds in the strip
	struct led_data ledstrip_data[num_leds];
	//init spi_led.c
	fd = init_spi_led(spi_speed);
	
	
	OutData = malloc(N/2 * sizeof(float));
	ret = gpu_fft_prepare(mb, NLOG2, GPU_FFT_FWD, jobs, &fft);
	bool logarithmic = false;
	//low for how many freq are used for the base, and high how many are used for the unused high parts
	int low, high, brightness;
	low = 300;
	high = 3000;
	brightness = 100;
	int freq_per_bin = rate/N;
	int low_bins, high_bins;
	
	//get how many bin are in the low part
	low_bins = round(low/freq_per_bin);
	
	//get how many bin are in the hight part
	high_bins = round((rate/2 - high)/freq_per_bin);
	
	for(int e=0; e<sf_info.frames/N;e++){
		base = fft->in;
		
		//read from audio device into buffer
		/*if ((err = snd_pcm_readi(capture_handle, buffer, buffer_frames)) != buffer_frames) {
			fprintf(stderr, "read from audio interface failed (%s)\n",
					err, snd_strerror(err));
			exit(1);
		}*/
		
		//Write the Data into the FFT Buffer
		for(i=0;i<N;i++){
			base[i].re = buf[i+e*N]/32768.0;
			base[i].im = 0.0;
			
			//Buffer for the playing the sound
			buffer[i] = buf[i+e*N];
		}
		
		//play the buffer
		rc = snd_pcm_writei(handle, buffer , frames);
		if (rc == -EPIPE) {
			/* EPIPE means underrun */
			fprintf(stderr, "underrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr,
					"error from writei: %s\n",
					snd_strerror(rc));
		}  else if (rc != (int)frames) {
			fprintf(stderr,
					"short write, write %d frames\n", rc);
		}
		
		//Perform FFT
		gpu_fft_execute(fft);
		
		base = fft->out;
		
		//read FFT output
		for(i=0;i<(N/2);i++)
		{
			// record the amplitude of this frequency bin
			OutData[i]= (2*sqrt((base[i].re * base[i].re) + (base[i].im * base[i].im)))/N;
			
			if (optimize && OutData[i] > max_amplitude && i > low_bins)
				max_amplitude = OutData[i];
			//printf("Loop %d Data point OutData[%d] = %f\n",e,(int) (rate/N)*i,OutData[i]);
		}
		
		
		if (max_amplitude)
		{
			amplitude_factor = 1.0/max_amplitude;
		}
		else
			amplitude_factor = 1.0;
		
		//Calculate the LEDs using one of the modes in led_modes
		mode1(N, OutData, low_bins, high_bins,  brightness, amplitude_factor, logarithmic, &ledstrip_data);
		//write the LED data to the LEDs
		write_spi_buffer(ledstrip_data, fd);
	}
	//Close everything
	gpu_fft_release(fft);
	return close_spi_led(fd);
}
