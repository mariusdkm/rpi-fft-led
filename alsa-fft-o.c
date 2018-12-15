/*
 Capturing from ALSA
 From Paul David's tutorial : http://equalarea.com/paul/alsa-audio.html
 
 sudo apt-get install libasound2-dev
 gcc -o alsa-record-example -lasound alsa-record-example.c && ./alsa-record-example hw:0
 
 GPU_FFT thanks to Andrew Holme
 
 other chunks of ccode copied wholesale from PeterO of raspberrypi.org, many thanks!
 */

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include <unistd.h>
#include <math.h>
#include <time.h>

#include "mailbox.h"
#include "gpu_fft.h"

main(int argc, char *argv[])
{
    
    
    int i,j;
    int err;
    int16_t *buffer;
    float* float_buffer;
    int buffer_frames = 128;
    unsigned int rate = 44100;
    char* devcID;
    snd_pcm_t *capture_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
    
    devcID = malloc(sizeof(char) * 5);
    if (argc > 1){
        devcID = argv[1];
    }
    else {
        devcID = "hw:0,0"; //doesn't seem to make a difference if this is just hw:0
    }
    
    if ((err = snd_pcm_open(&capture_handle, devcID, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "cannot open audio device %s (%s)\n",devcID,snd_strerror(err));
        exit(1);
    } else {fprintf(stdout, "audio interface opened\n");}
    
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params allocated\n"); }
    
    if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
        fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params initialized\n"); }
    
    if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "cannot set access type (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params access set\n"); }
    
    if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
        fprintf(stderr, "cannot set sample format (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params format set\n"); }
    
    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
        fprintf(stderr, "cannot set sample rate (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params rate set\n"); }
    
    if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1)) < 0) {
        fprintf(stderr, "cannot set channel count (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params channels set\n"); }
    
    if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
        fprintf(stderr, "cannot set parameters (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "hw_params set\n"); }
    
    
    snd_pcm_hw_params_free(hw_params);
    
    fprintf(stdout, "hw_params freed\n");
    
    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror(err));
        exit(1);
    }    else { fprintf(stdout, "audio interface prepared\n"); }
    
    //allocate buffer of 16bit ints, as specified in PCM_FORMAT
    buffer = malloc(buffer_frames * snd_pcm_format_width(format) / 8 * 2);
    float_buffer = malloc(buffer_frames*sizeof(float));
    
    
    
    fprintf(stdout, "buffer allocated\n");
    
    float *OutData;
    int NLOG2 = 10;
    unsigned long N = 1 << NLOG2;
    OutData = malloc(N * sizeof(float));
    int Peaki;
    float PeakValue;
    
    for (i = 0; i < 10000; ++i) {
        //read from audio device into buffer
        if ((err = snd_pcm_readi(capture_handle, buffer, buffer_frames)) != buffer_frames) {
            fprintf(stderr, "read from audio interface failed (%s)\n",
                    err, snd_strerror(err));
            exit(1);
        }
        //try to change buffer from short ints to floats for FFT transformation
        for (i = 0; i < buffer_frames; i++){
            float_buffer[i] = (float)buffer[i];
        }
        
        //printf("Compute FFT  ... ");
        fft_compute_forward(buffer, NLOG2, OutData, 1);
        
        printf("\n");
        usleep(100);
    }
    
    
    free(buffer);
    free(OutData);
    
    fprintf(stdout, "memory freed\n");
    
    snd_pcm_close(capture_handle);
    fprintf(stdout, "audio interface closed\n");
    fprintf(stdout,"Press enter to continue\n");
    getchar();
    return(0);
    exit(0);
}

int fft_compute_forward(float * input, int log2_N, float * output, double sampling_interval)
{
    int mb = mbox_open();
    int jobs = 1, i;
    unsigned long N = 1 << log2_N;
    
    float FloatN = (float)N;
    float HalfN = FloatN / 2.0;
    struct GPU_FFT_COMPLEX * base;
    struct GPU_FFT * fft;
    struct GPU_FFT_COMPLEX *DataIn, *DataOut;
    int ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_FWD, jobs, &fft);
    base = fft->in;
    
    for (i = 0; i<N; i++)
    {
        base[i].re = input[i]; base[i].im = 0.0;
    }
    
    gpu_fft_execute(fft);
    base = fft->out;
    
    for (i = 0; i<N; i++)
        output[i] = (base[i].re * base[i].re) + (base[i].im * base[i].im);
    
    gpu_fft_release(fft);
    mbox_close(mb);
}
