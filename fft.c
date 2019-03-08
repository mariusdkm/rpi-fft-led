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

//For multithreading
#include <pthread.h>

//to get the maximum number
#include <limits.h>

// for reading wav files:
#include <sndfile.h>

//GPU_FFT librarys
#include "/opt/vc/src/hello_pi/hello_fft/mailbox.h"
#include "/opt/vc/src/hello_pi/hello_fft/gpu_fft.h"

#include "spi_led.h"
#include "led_modes.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
//ALSA library
#include <alsa/asoundlib.h>

//Bool to indicate if the Buffer is full
bool fulldata = false;
int16_t *buffer;

//Stuct for the Audiodevice
typedef struct
{
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int rate;
	int dir;
	snd_pcm_uframes_t frames;
	snd_pcm_format_t format;
}ad;

//Function to get preciced time mesarements implemented from hello_fft.c
unsigned Microseconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec*1000000 + ts.tv_nsec/1000;
}
//Function to read the Data. Will be run in a seperate Thread
void *readData(void *audio_device){
	int err;
	ad *audiodevice = (ad *) audio_device;
	printf("Hello from audio Thread %d\n", (int) getpid());
	while(true){
		while(fulldata);
		if ((err = snd_pcm_readi(audiodevice->handle, buffer, audiodevice->frames)) != audiodevice->frames) {
			fprintf(stderr, "read from audio interface failed (%s)\n",
					err, snd_strerror(err));
			exit(1);
		}
		fulldata = true;
	}
}


int main(int argc, char *argv[]) {
	//How many samples are used has to be 2^X 2^11=2048 works the best.
	int NLOG2 = 11;
	unsigned long N = 1 << NLOG2;
	
	//loop is used to control how often the analyzing is going
	int loop = INT_MAX;
	
	//Read arguments *has to be optimized*
	int mode = 2;
	bool mic = false;
	bool optimize = false;
	bool logarithmic = false;
	printf("N=%d\n", N);
	if(!strcmp(argv[1], "mic"))
	{
		mic = true;
		printf("microphone enabled\n");
	} else {
		printf("soundfile enabled\n");
	}
	if (argc > 2)
	{
		mode = (int) (*argv[2]-'1');
	}
	if (argc > 3){
		if(!strcmp(argv[3], "log"))
		{
			logarithmic = true;
			printf("logarithmic enabled\n");
		} else {
			optimize = true;
			printf("optimizing enabled\n");
		}
	}
	printf("mode %d enabled\n",mode);
	
	//Open the WAV file.
	if (argc < 2 || argc > 5)
		pabort("Call me with the name of the sound file as argument or mic if you want to use a microphone and if you want a second argument for the LED mode (and optionally a third  argument to optimize display for this sound file)");
	
	//ALSA audio device init-------------------
	int err;
	int rc;
	int size;
	
	ad audio_device;
	audio_device.format = SND_PCM_FORMAT_S16_LE;
	if(mic)
	/* Open PCM device for recording. */
		rc = snd_pcm_open(&audio_device.handle, "hw:1,0", SND_PCM_STREAM_CAPTURE, 0);
	else
	/* Open PCM device for playback. */
		rc = snd_pcm_open(&audio_device.handle, "hw:0,1", SND_PCM_STREAM_PLAYBACK, 0);
	
	if (rc < 0) {
		fprintf(stderr,
				"unable to open pcm device: %s\n",
				snd_strerror(rc));
		exit(1);
	}
	
	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&audio_device.params);
	
	/* Fill it in with default values. */
	snd_pcm_hw_params_any(audio_device.handle, audio_device.params);
	
	/* Set the desired hardware parameters. */
	
	/* Interleaved mode */
	snd_pcm_hw_params_set_access(audio_device.handle, audio_device.params,SND_PCM_ACCESS_RW_INTERLEAVED);
	
	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(audio_device.handle, audio_device.params, audio_device.format);
	
	/* One Channel (Mono) */
	snd_pcm_hw_params_set_channels(audio_device.handle, audio_device.params, 1);
	
	/* 44100 bits/second sampling rate (CD quality) */
	audio_device.rate = 44100;
	snd_pcm_hw_params_set_rate_near(audio_device.handle, audio_device.params, &audio_device.rate, &audio_device.dir);
	
	/* Set period size to N frames. */
	audio_device.frames = N;
	snd_pcm_hw_params_set_period_size_near(audio_device.handle, audio_device.params, &audio_device.frames, &audio_device.dir);
	
	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(audio_device.handle, audio_device.params);
	if (rc < 0) {
		fprintf(stderr,
				"unable to set hw parameters: %s\n",
				snd_strerror(rc));
		exit(1);
	}
	
	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(audio_device.params, &audio_device.frames,
									  &audio_device.dir);
	
	//allocate buffer of 16bit ints, as specified in PCM_FORMAT
	buffer = malloc(audio_device.frames * snd_pcm_format_width(audio_device.format) / 8 * 2);
	
	//File reading code-----------------------
	short *buf;
	if(!mic){
		SNDFILE *sf;
		SF_INFO sf_info;
		int num_items;
		FILE *out;
		
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
		printf("Read %d items\n", sf_read_short(sf,buf,num_items));
		sf_close(sf);
		// code below only works for mono files, so abort here if not mono
		if (sf_info.channels > 1)
			pabort("Please use a mono wav file, not stereo.");
		
		loop = sf_info.frames/N;
		fulldata = true;
	}
	// ----------------------------------------------
	
	
	
	int ret = 0;
	int fd, i;
	
	//FFT Variables
	float * OutData;
	OutData = malloc(N/2 * sizeof(float));
	
	int jobs=1, mb = mbox_open();
	
	struct GPU_FFT_COMPLEX * base;
	struct GPU_FFT * fft;
	struct GPU_FFT_COMPLEX *DataIn, *DataOut;
	
	
	//Variables used for timing
	unsigned t[7];
	int total = 0;
	
	//low for how many freq are used for the base, and high how many are used for the unused high parts
	int low, high, brightness;
	low = 200;
	//high = 2530*4;
	high = 1500;
	//high = 800;
	brightness = 70;
	float freq_per_bin= audio_device.rate * 1.0/N;
	
	int low_bins, high_bins;
	
	//get how many bin are in the low part
	low_bins = round(low/freq_per_bin);
	//get how many bin are in the hight part
	high_bins = round((audio_device.rate/2-high)/freq_per_bin);
	
	// a structure encoding the rgb value for all of the leds in the strip
	struct led_data ledstrip_data[num_leds];
	
	
	
	//Variables used for the optimize feature
	float max_amplitude = 0,amplitude_factor;
	
	//init spi_led.c-----------------------
	uint32_t spi_speed = 4000000; //=250ns
	fd = init_spi_led(spi_speed);
	//-------------------------------------
	
	//set all LED to black(0,0,0) to start
	led_data led_zero = {
		.r = 0,
		.g = 0,
		.b = 0
	};
	for(i=0; i<num_leds; i++)
		ledstrip_data[i]=led_zero;
	write_spi_buffer(ledstrip_data, fd);
	
	//Array-Struct with all the LED-Modes
	typedef void (*led_mode)(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]);
	
	led_mode led_modes[4] = {&mode1, &mode2, &mode3, &mode4};
	
	
	ret = gpu_fft_prepare(mb, NLOG2, GPU_FFT_FWD, jobs, &fft);
	
	switch(ret) {
		case -1: printf("Unable to enable V3D. Please check your firmware is up to date.\n"); return -1;
		case -2: printf("log2_N=%d not supported.  Try between 8 and 22.\n", NLOG2);         return -1;
		case -3: printf("Out of memory.  Try a smaller batch or increase GPU memory.\n");     return -1;
		case -4: printf("Unable to map Videocore peripherals into ARM memory space.\n");      return -1;
		case -5: printf("Can't open libbcm_host.\n");                                         return -1;
	}
	
	
	
	if(mic){
		pthread_t audio_thread;
		//read from audio device into buffer
		rc = pthread_create(&audio_thread, NULL, readData, &audio_device);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	
	printf("Hello from main Thread %d\n", (int) getpid());
	
	for(int e=0; e<loop;e++){
		
		base = fft->in;
		//Write the Data into the FFT Buffer
		t[0] = Microseconds();
		while(!fulldata)
		{
			usleep(1);
		}
		
		t[1] = Microseconds();
		for(i=0;i<N;i++){
			base[i].im = 0.0;
			if(mic){
				base[i].re = buffer[i]/32768.0;
			}else{
				base[i].re = buf[i+e*N]/32768.0;
				//Buffer for the playing the sound
				buffer[i] = buf[i+e*N];
			}
			
		}
		fulldata = false;
		
		t[2] = Microseconds();
		
		if(!mic){
			fulldata = true;
			//play the buffer
			rc = snd_pcm_writei(audio_device.handle, buffer , audio_device.frames);
			if (rc == -EPIPE) {
				// EPIPE means underrun
				fprintf(stderr, "underrun occurred\n");
				snd_pcm_prepare(audio_device.handle);
			} else if (rc < 0) {
				fprintf(stderr,
						"error from writei: %s\n",
						snd_strerror(rc));
			}  else if (rc != (int)audio_device.frames) {
				fprintf(stderr,
						"short write, write %d frames\n", rc);
			}
		}
		
		//Perform FFT
		gpu_fft_execute(fft);
		
		t[3] = Microseconds();
		
		base = fft->out;
		
		//read FFT output
		for(i=0;i<(N/2);i++)
		{
			// record the amplitude of this frequency bin
			OutData[i]= (2*sqrt((base[i].re * base[i].re) + (base[i].im * base[i].im)))/N;
			//printf("Loop %d Data point OutData[%d] = %f\n",e,(int) (audio_device.rate/N)*i,OutData[i]);
			
			//Optimization feature
			if (optimize && i > low_bins)
				max_amplitude = OutData[i];
		}
		t[4] = Microseconds();
		
		if (max_amplitude)
		{
			amplitude_factor = 1.0/max_amplitude;
		}
		else
			amplitude_factor = 1.0;
		//Calculate the LEDs using one of the modes in led_modes
		led_modes[mode](N, OutData, low_bins, high_bins,  brightness, amplitude_factor, logarithmic,(void *) &ledstrip_data);
		t[5] = Microseconds();
		//write the LED data to the LEDs
		write_spi_buffer(ledstrip_data, fd);
		t[6] = Microseconds();
		if((t[1]-t[0]) > 45056){
			total +=1;
			printf("\e[43;31mOvertime at e = %d it's the %d. time\e[0m\n",e, total);
		}
		//Output to see timing
		/*
		printf("Audio wait=%d, Audio read=%d, FFT=%d, FFT-Output read=%d, LED calculation=%d, LED write=%d\n", t[1]-t[0],t[2]-t[1],t[3]-t[2],t[4]-t[3],t[5]-t[4], t[6]-t[5]);
		printf("TOTAL=%d\n",t[6]-t[0]);
		for(i = 0; i < num_leds; i++)
			printf("For LED %d wrote r=%d g=%d b=%d\n",i, ledstrip_data[i].r, ledstrip_data[i].g,ledstrip_data[i].b);
		 */
	}
//Close everything
gpu_fft_release(fft);
return close_spi_led(fd);
}
