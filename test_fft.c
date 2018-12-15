/*
 BCM2835 "GPU_FFT" release 2.0 BETA
 Copyright (c) 2014, Andrew Holme.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the copyright holder nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "mailbox.h"
#include "gpu_fft.h"

int fft_compute_forward(float * input , int log2_N, float * output, int direction , double sampling_interval)
{
    int mb = mbox_open();
    int jobs=1, i;
    unsigned long N = 1 << log2_N;
    
    float FloatN = (float) N;
    float HalfN  = FloatN / 2.0;
    struct GPU_FFT_COMPLEX * base;
    struct GPU_FFT * fft;
    struct GPU_FFT_COMPLEX *DataIn, *DataOut;
    int ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_FWD, jobs, &fft);
    base = fft->in;
    
    for(i=0;i<N;i++)
    {
        base[i].re =  input[i];
        base[i].im =0.0;
    }
    
    gpu_fft_execute(fft);
    base = fft->out;
    FILE *fpv;
    fpv = fopen("/home/pi/hello_fft/400hz.data", "w"); // open text file
    printf("\n");
    for(i=0;i<N;i++){
        output[i]= sqrt(base[i].re * base[i].re) + (base[i].im * base[i].im);
        //printf("%d\t%d\t%d\n", base[i].re, base[i].im, output[i]);
        fprintf(fpv, "%d\t%d\t%d\n", base[i].re, base[i].im, output[i]);
    }
    fclose(fpv);
    gpu_fft_release(fft);
    mbox_close(mb);
}


float sinewave(float indice, float Frequency,unsigned long N)
{
    float Factor = Frequency  * 2.0 * 3.141592654/ N;
    return sin( Factor  * indice);
}



int main(int argc, char *argv[]) {
    
    float * InData, * OutData;
    float Amplitude;
    float Frequency;
    int  loop,i;
    int Peaki;
    float PeakValue;
    FILE *fp;
    fp = fopen("/home/pi/hello_fft/400hz.csv", "r"); // open text file
    
    
    int NLOG2 = 10;
    unsigned long N = 1 << NLOG2;
    
    
    InData = malloc(N * sizeof(float));
    OutData = malloc(N * sizeof(float));
    
    
    for(int v= 0; v < 1; v++)
    {
    
        // GENERATE SINE WAVE
        if(0){
            // create dummy  random * F sine wave
            
            // first select amplitude
            Amplitude = (float) (rand() % 100);
            Frequency = (float) (rand() % (N/2));
            FILE *fp;
            fp = fopen("/home/pi/hello_fft/piano.data", "w+");
            printf("======================\n");
            printf("Create sine wave  Frequency= %0.f  Amplitude= %0.f\n",Frequency,Amplitude);
            printf("AMP %f", Amplitude);
            for(i=0;i<N;i++){
                InData[i]=  Amplitude * sinewave(i,Frequency,N);
                printf("InData %d: %f\n",i ,InData[i]);
                
                if(0){ //write Data in File
                    
                    fprintf(fp, "%f\n", InData[i]);
                    //fputs("\n", fp);
                }
            }
            fclose(fp);
        }
    
        // READ TONES FROM FILE
        if(1){
            
            
            char buff[255];
            
            printf("======================\n");
            printf("Reading data:\n");
            
            for(i=0;i<N;i++){
                fgets(buff, 255, (FILE*)fp);
                //printf("Test: %s", buff );
                InData[i]=  atof(buff);
                
                //printf("InData %d: %f\n",i ,InData[i]);
            }
            
        }
    

        printf("Compute FFT  ... ");
        fft_compute_forward(InData, NLOG2, OutData, 1,1);
        printf("Done\n");
        
        // now from the FFT find the Peak
        
        Peaki=0;
        PeakValue=0;
        for(i=1;i<=(N/2);i++)
        {
            //printf("Output Freq:%d  Altitute: %f \n", (i*44100/1024) , (sqrt(OutData[i])/ (N/2.0)));
            if(OutData[i] > PeakValue)
            {
                //printf("Output %d: %f N: %d \n", i , OutData[i], N);
                PeakValue= OutData[i];
                Peaki = i;
            }
        }
        printf("FFT Found Peak at Frequency= %d Amplitude = %.0f\n\n",\
               (Peaki*44100/1024), PeakValue);
    }
    fclose(fp);

    free(InData);
    free(OutData);
    return 0;
}
