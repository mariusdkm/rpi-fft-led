//
//  led_modes.c
//  
//
//  Created by Marius Dario Cora on 05.01.19.
//

#include "led_modes.h"



// auxiliary function mapping spectral frequency to rgb values, based on
// https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum
//Guess the log scale to a genauigkeit to 7 decimal places
static float log_scale(int width, int range){
	static float log, sum, log_scale, min= 10.0, decimal = 0.1;
	static int i;
	for(log = 1; log < pow(range, (1.0/width)); log+=1*decimal){
		sum = 0.0;
		for(i = 0; i < width; i++){
			sum += pow(log, i);
		}
		printf("log= %f\n", log);
		if(abs(range-sum) < min ){
			min = abs(range-sum);
			log_scale = log;
			printf("log_scale= %f\n", log_scale);
			if(min == 0.0)
				return log_scale;
		}
		if(log==9 || log > pow(range, (1.0/width))){
			decimal*=0.1;
			log = log_scale;
		}
	}
	return log_scale;
}

static struct led_data spectral_color(double l, int brightness) { // returns RGB = 0-1; l = 400-700 nm wavelenght

	double t, r=0.0, g=0.0, b=0.0;
	
	//	printf("   frequency l= %f ", l);
	if ((l>=400.0)&&(l<410.0)) { t=(l-400.0)/(410.0-400.0); r=    +(0.33*t)-(0.20*t*t); }
	else if ((l>=410.0)&&(l<475.0)) { t=(l-410.0)/(475.0-410.0); r=0.14         -(0.13*t*t); }
	else if ((l>=545.0)&&(l<595.0)) { t=(l-545.0)/(595.0-545.0); r=    +(1.98*t)-(     t*t); }
	else if ((l>=595.0)&&(l<650.0)) { t=(l-595.0)/(650.0-595.0); r=0.98+(0.06*t)-(0.40*t*t); }
	else if ((l>=650.0)&&(l<700.0)) { t=(l-650.0)/(700.0-650.0); r=0.65-(0.84*t)+(0.20*t*t); }
	if ((l>=415.0)&&(l<475.0)) { t=(l-415.0)/(475.0-415.0); g=             +(0.80*t*t); }
	else if ((l>=475.0)&&(l<590.0)) { t=(l-475.0)/(590.0-475.0); g=0.8 +(0.76*t)-(0.80*t*t); }
	else if ((l>=585.0)&&(l<639.0)) { t=(l-585.0)/(639.0-585.0); g=0.84-(0.84*t)           ; }
	if ((l>=400.0)&&(l<475.0)) { t=(l-400.0)/(475.0-400.0); b=    +(2.20*t)-(1.50*t*t); }
	else if ((l>=475.0)&&(l<560.0)) { t=(l-475.0)/(560.0-475.0); b=0.7 -(     t)+(0.30*t*t); }
	
	//printf("results in values: r= %f, g= %F, b= %f\n", r, g, b);
	
	//while setting the  led_data, also scale each value to 255 (one byte) and add brightness(scale down to 10 because 100 would be to much)
	
	led_data rgb_data = {
		.r = (uint8_t) round(r * 255/(10-(brightness*0.1))),
		.g = (uint8_t) round(g * 255/(10-(brightness*0.1))),
		.b = (uint8_t) round(b * 255/(10-(brightness*0.1)))
	};
	
	return rgb_data;
}
//In this mode sets a rainbow along teh ledstrip and assing bins to leds the brightness is the amplitude only the highs are cut off
void mode1(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//cut of highs
	int mid_bins = N/2 - high_bins;
	static int scaling, amplitude, overflow;
	static int current_led, current_bin, bin;
	static float max, steps;
	if(logarithmic)
		printf("No formula jet");//No formula jet
	else
		scaling = round(mid_bins/num_leds); //Calculate how many bins per LED
	
	overflow = round(num_leds/(mid_bins-(scaling*num_leds)));
	//printf("scaling = %d overflow = %d\n",scaling, overflow);
	steps = 200/num_leds; //Calculate what light freq per LED
	bin = 0;
	for(current_led=0;current_led<num_leds;current_led++)
	{
		max = 0;
		// as value for an led, we use the highest value of one of its FFT frequency bins
		// (in principle, using the average may be better, but if we have many bins that we sum over and that destroys the accuray
		//the  current_led % overflow adds the missing bins by using the modulus operator
		for (current_bin=0;current_bin<scaling + (current_led % overflow);current_bin++)
		{
			//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, scaling + (current_led % overflow), current_bin ,bin);
			//printf("Data[%d] = %f\n", bin, Data[bin]);
			if (Data[bin] > max)
				max = Data[bin];
			bin++;
			
		}
		// scale so that the maximum amplitude is 1 and so that the brighness ist calculated
		max *= amplitude_factor * brightness;
		//write led data into the data files
		//printf("light freq=%f\n", steps*current_led);
		(*ledstrip_data)[current_led] = spectral_color(steps*current_led+450, max);
	}
}

void mode2(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//cut of highs
	int mid_bins = N/2-high_bins;
	static int scaling, amplitude, overflow;
	static int current_led, current_bin, bin, num_bins;
	static float max;
	if(true){
		scaling = round(log_scale(num_leds, mid_bins));
		printf("log_scale returned: %f\n", log_scale(num_leds, N/2));
	}else
		scaling = round(mid_bins/num_leds); //Calculate how many bins per LED
	scaling = (int) floor(mid_bins/num_leds);
	overflow = (int) round(num_leds/(mid_bins-(scaling*num_leds*1.0)));
	printf("scaling = %doverflow = %d\n",scaling, overflow);
	bin = 0;
	for(current_led=0;current_led<num_leds;current_led++)
	{
		max = 0;
		// as value for an led, we use the highest value of one of its FFT frequency bins
		// (in principle, using the average may be better, but if we have many bins that we sum over and that destroys the accuray
		//the  current_led % overflow adds the missing bins by using the modulus operator
		if((current_led % overflow) == 0)
			num_bins = scaling + 1;
		else
			num_bins = scaling;
			
		for (current_bin=0;current_bin<num_bins;current_bin++)
		{
			//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, num_bins, current_bin ,bin);
			//printf("Data[%d] = %f\n", bin, Data[bin]);
			if (Data[bin] > max)
				max = Data[bin];
			bin++;
			
		}
		// scale so that the maximum amplitude of the data file is 1
		max *= amplitude_factor;
		// with the amplitude values between 0 and 1, we want to realize
		// visible spectrum color from 450 to 600 nm:
		amplitude = max*200.0+450;
		//write led data into the data file
		//printf("amplitude: %d, max = %f\n", amplitude, max);
		(*ledstrip_data)[current_led] = spectral_color(amplitude, brightness);
		//printf("Returning: r %d, g %d, b %d\n", (*ledstrip_data)[current_led].r,(*ledstrip_data)[current_led].g,(*ledstrip_data)[current_led].b);
		//printf(" max for led %d: %f (freq = %f)\n",current_led,max,amplitude);
	}
}

void mode3(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//rintf("Using LED Mode 3\n");
	//cut of highs and low bins bc they are used for brightness
	int mid_bins = N/2 - high_bins- low_bins;
	static int scaling, amplitude, overflow, bass_max = 30;
	static int current_led, current_bin, bin, num_bins;
	static float max;
	if(logarithmic){
		scaling = round(log_scale(num_leds, mid_bins));
		printf("log_scale returned: %f\n", log_scale(num_leds, N/2));
	}else
		scaling = round(mid_bins/num_leds); //Calculate how many bins per LED
	scaling = (int) floor(mid_bins/num_leds);
	overflow = (int) round(num_leds/(mid_bins-(scaling*num_leds*1.0)));
	//printf("scaling = %d overflow = %d\n",scaling, overflow);
	bin = 0;
	for (current_bin=1;current_bin<low_bins;current_bin++)
	{
		max = 0;
		//printf("For Bass: %d bins current bin: %d of %d bins\n", scaling + (current_led % overflow), current_bin ,bin);
		if (Data[bin] > max)
			max = Data[bin];
		
		
	}
	//printf("bass:%f\n", max);
	//set brightness by * multipling with the low_bins max
	brightness = round(max * 100);
	//printf("bass:%f brightnees:%d bass_max:%d\n", max, brightness, bass_max);
	//since we already started using the bin vaiable we don't have to add the offset
	for(current_led=0;current_led<num_leds;current_led++)
	{
		max = 0;
		// as value for an led, we use the highest value of one of its FFT frequency bins
		// (in principle, using the average may be better, but if we have many bins that we sum over and
		if((current_led % overflow) == 0)
			num_bins = scaling + 1;
		else
			num_bins = scaling;
		
		for (current_bin=0;current_bin<num_bins;current_bin++)
		{
			//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, num_bins, current_bin ,bin);
			if (Data[bin] > max)
				max = Data[bin];
			bin++;
			
		}
		// scale so that the maximum amplitude of the data file is 1
		max *= amplitude_factor;
		// with the amplitude values between 0 and 1, we want to realize
		// visible spectrum color from 450 to 600 nm:
		amplitude = max*200.0+450;
		//write led data into the data file
		(*ledstrip_data)[current_led] = spectral_color(amplitude, brightness);
		
		//printf(" max for led %d: %f (freq = %f)\n",current_led,max,amplitude);
	}
	
}


/*
// How many fft bins do we have?  HalfN
// How many leds do we have? num_leds
// Therefore, how many fft bins need to be averaged for each led?
if(LOGARITHMIC)
lin_bins = (N/2)/num_leds
log_scaling = 1.17455;
//freq_scaling = pow((N/16.0), (1.0/num_leds));
//printf("freq_scaling: %f\n", freq_scaling);
num_bins = (int) (round((N/2)/num_leds));
//base = num_leds root of num_bins
// im loop
int bin = 0;
//low for how many freq are used for the base, and high how many are used for the upper part
int low, high, brightness;
low = 100;
high = 3000;
brightness = 99;
int low_bins, mid_bins, high_bins;
//(rate/(N/2)) = how many freq are in one bin
//get how many bin are in the low part
for(i = 0; i<(N/2); i++)
if((rate/(N/2)*i>=low)
   low_bins = i;
   //get how many bin are in the hight part
   for(i = (rate/N)*(N/2); i>0; i--)
   if((rate/(N/2))*i<=high)
   high_bins =(N/2)-i;
   
   //Calculate mid bins
   mid_bins = (N/2) - low_bins - high_bins;
   
   //this code has to get it's own funktion
   for(current_led=0;current_led<num_leds;current_led++)
   {
	   
	   
	   //printf(" max for led %d: %f (freq = %f)\n",current_led,max,amplitude);
   }
   
*/
