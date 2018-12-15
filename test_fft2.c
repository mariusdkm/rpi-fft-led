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
    int ret = gpu_fft_prepare(mb, log2_N, direction == 1 ? GPU_FFT_FWD : GPU_FFT_REV , jobs, &fft);
    base = fft->in;
    
    for(i=0;i<N;i++)
    {
        base[i].re =  input[i];
        base[i].im =0.0;
    }
    
    gpu_fft_execute(fft);
    base = fft->out;
    
    for(i=0;i<N;i++)
        output[i]= (base[i].re * base[i].re) + (base[i].im * base[i].im);
    
    gpu_fft_release(fft);
    mbox_close(mb);
}

int main(int argc, char *argv[]) {
    
    float * InData, * OutData;
    float Amplitude;
    float Frequency;
    int  loop,i,length=10;
    int Peaki;
    float PeakValue;
    FILE *fp;
    fp = fopen("/home/pi/hello_fft/tone.data", "r"); // open text file
    
    
    int NLOG2 = 10;
    unsigned long N = 1 << NLOG2;
    
    
    InData = malloc(N * sizeof(float));
    OutData = malloc(N * sizeof(float));
    
    
    for(int v= 0; v < length; v++)
    {
        // READ TONES FROM FILE
        char buff[255];
        printf("======================\n");
        printf("Reading data:\n");
            
        for(i=0;i<N;i++){
            fgets(buff, 255, (FILE*)fp);
            InData[i]=  atof(buff);
            }
    

        printf("Compute FFT  ... ");
        fft_compute_forward(InData, NLOG2, OutData, 1,1);
        printf("Done\n");
        
        // now from the FFT find the Peak
        
        Peaki=0;
        PeakValue=0;
        for(i=1;i<=N;i++)
        {
            if(OutData[i] > PeakValue)
            {
                PeakValue= OutData[i];
            }
        }
        printf("FFT Found Peak at Frequency= %d Amplitude = %.0f\n\n",\
               Peaki, sqrt(PeakValue)/ (N/2.0));
    }
    fclose(fp);
    free(InData);
    free(OutData);
    return 0;
}
