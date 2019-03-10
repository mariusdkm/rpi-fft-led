//
//  led_modes.c
//

#include "led_modes.h"

//Approqimate the base for the logarithmic scaling to a accuracy to 7 decimal places
static float log_scale(int width, int range){
	static int st, num, i;
	float log = 1.0, min = range*1.0, sum;
	int min_rnd = range, sum_rnd;
	for(st = 1; st < 7;st++){
		for(num = 0; num<10; num++){
			sum = 0.0, sum_rnd = 0;
			log+=pow(10, -1*st);
			for(i = 0; i < width; i++){
				sum += pow(log, i);
				sum_rnd += round(pow(log, i));
			}
			if((abs(range*1.0-sum) <= min) && (abs(range-sum_rnd) <= min_rnd)){
				min = abs(range*1.0-sum);
				min_rnd = abs(range - sum_rnd);
			}else{
				log-=1.5*pow(10, -1*st);
				sum = 0.0, sum_rnd = 0;
				for(i = 0; i < width; i++){
					sum += pow(log, i);
					sum_rnd += round(pow(log, i));
				}
				min = abs(range*1.0-sum);
				min_rnd = abs(range - sum_rnd);
				break;
			}
		}
	}
	return log;
}


// auxiliary function mapping spectral frequency to rgb values, based on
// https://stackoverflow.com/questions/3407942/rgb-values-of-visible-spectrum
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
	
	//printf("results in values: l=%f r= %f, g= %F, b= %f\n",l, r, g, b);
	
	//while setting the  led_data, add brightness(0-10)
	led_data rgb_data = {
		.r = (uint8_t) round(r * (brightness*2.55)),
		.g = (uint8_t) round(g * (brightness*2.55)),
		.b = (uint8_t) round(b * (brightness*2.55))
	};
	//printf("Returning: r %d, g %d, b %d\n", rgb_data.r,rgb_data.g,rgb_data.b);
	return rgb_data;
}


//In this mode sets a rainbow along the ledstrip and assing bins to leds the brightness is the amplitude only the highs are cut off
void mode1(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//cut of highs
	int mid_bins = N/2 - high_bins;
	//mid_bins = 235;
	static int scaling, amplitude, overflow;
	static int current_led, current_bin, bin, num_bins;
	float max, steps;
	if(logarithmic){
		printf("Not supported in this modes jet");
	}else{}
	
	scaling = (int) floor(mid_bins/num_leds);
	overflow = (int) round(num_leds/(mid_bins-(scaling*num_leds*1.0)));
	//printf("scaling = %d overflow = %d\n",scaling, overflow);
	steps = 200.0/num_leds; //Calculate what light freq per LED
	//printf("steps:%f", steps);
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
		
		if(num_bins==0)
			max = Data[bin];
		for (current_bin=0;current_bin<num_bins;current_bin++)
		{
			//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, scaling + (current_led % overflow), current_bin ,bin);
			if (Data[bin] > max)
				max = Data[bin];
			bin++;
			
		}
		
		//Filtering unheareble parts out
		if(max < 0.02)
			max = 0.00001;
		
		// scale so that the maximum amplitude is 1 and so that the brighness is calculated
		max *= 200 * amplitude_factor;
		
		//write led data into Struct
		(*ledstrip_data)[current_led] = spectral_color(steps*current_led+450, max);
	}
}


void mode2(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//cut of highs
	int mid_bins = N/2-high_bins;
	//printf("mid_bins=%d\n", mid_bins);
	
	static int scaling, amplitude, overflow;
	static int current_led, current_bin, bin, num_bins;
	static float max;
	
	if(logarithmic){
		static float scaling_log = 0.0;
		if(scaling_log == 0.0){
			scaling_log = log_scale(num_leds, mid_bins);
			printf("log_scale returned: %f\n", scaling_log);
		}
		
		bin = 0;
		for(current_led=0;current_led<num_leds;current_led++)
		{
			max = 0;
			/*
			 As value for an led, we use the highest value of one of its FFT frequency bins
			 (in principle, using the average may be better,
			 but if we have many bins that we sum over and that destroys the accuray
			 num_bins represents how many bins are mapped to current_led
			 */
			num_bins = round(pow(scaling_log, current_led));
			
			for (current_bin=0;current_bin<num_bins;current_bin++)
			{
				//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, num_bins, current_bin ,bin);
				if (Data[bin] > max)
					max = Data[bin];
				bin++;
				
			}
			//Filtering unheareble parts out
			if(max < 0.02)
				max = 0.00001;
			
			// opmization feature
			max *= amplitude_factor;
			
			// with the amplitude values between 0 and 1, we want to realize
			// visible spectrum color from 400 to 700 nm
			// But the Output between 400-450 is very dark but that is good
			//because we don't want to see low amplitudes
			amplitude = max*300.0+400;
			
			//write led data into struct
			(*ledstrip_data)[current_led] = spectral_color(amplitude, brightness);
			//printf("Returning: r %d, g %d, b %d\n", (*ledstrip_data)[current_led].r,(*ledstrip_data)[current_led].g,(*ledstrip_data)[current_led].b);
		}
	}else{
		scaling = (int) floor(mid_bins/num_leds); //Calculate how many bins per LED
		overflow = (int) round(num_leds/(mid_bins-(scaling*num_leds*1.0)));
		//printf("scaling = %doverflow = %d\n",scaling, overflow);
		bin = 0;
		for(current_led=0;current_led<num_leds;current_led++)
		{
			/*
			 As value for an led, we use the highest value of one of its FFT frequency bins
			 (in principle, using the average may be better,
			 but if we have many bins that we sum over and that destroys the accuray)
			 the  current_led % overflow adds the missing bins by using the modulus operator
			 */
			if((current_led % overflow) == 0)
				num_bins = scaling + 1;
			else
				num_bins = scaling;
			
			//If no bin is mapped to a LED we use the data the previous LED => we don't reset max
			if(num_bins != 0)
				max = 0;
			
			for (current_bin=0;current_bin<num_bins;current_bin++)
			{
				//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, num_bins, current_bin ,bin);
				//printf("Data[%d] = %f\n", bin, Data[bin]);
				if (Data[bin] > max)
					max = Data[bin];
				bin++;
				
			}
			//Filtering unheareble parts out
			if(max < 0.02)
				max = 0.00001;
			
			// opmization feature
			max *= amplitude_factor;
			
			// with the amplitude values between 0 and 1, we want to realize
			// visible spectrum color from 400 to 700 nm
			// But the Output between 400-450 is very dark but that is good
			//because we don't want to see low amplitudes
			amplitude = max*300.0+400;
			
			//write led data into struct
			(*ledstrip_data)[current_led] = spectral_color(amplitude, brightness);
			//printf("Returning: r %d, g %d, b %d\n", (*ledstrip_data)[current_led].r,(*ledstrip_data)[current_led].g,(*ledstrip_data)[current_led].b)
		}
	}
}


void mode3(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//rintf("Using LED Mode 3\n");
	//cut of highs and low bins bc lows they are used for brightness
	int mid_bins = N/2 - high_bins- low_bins;
	static int scaling, amplitude, overflow;
	static int current_led, current_bin, bin, num_bins;
	static float max;
	
	if(logarithmic){
		printf("Not supported in this modes jet");
	}else{}
	
	scaling = (int) floor(mid_bins/num_leds); //Calculate how many bins per LED
	overflow = (int) round(num_leds/(mid_bins-(scaling*num_leds*1.0)));
	//printf("scaling = %d overflow = %d\n",scaling, overflow);
	
	//reset
	bin = 0;
	max = 0;
	
	for (current_bin=1;current_bin<low_bins;current_bin++)
	{
		//printf("For Bass: %d bins current bin: %d of %d bins\n", scaling + (current_led % overflow), current_bin ,bin);
		max += Data[bin];
		bin++;
	}
	//set brightness by * multipling max with 200 => the Bass adds extra brightness
	brightness = round(max * 200);
	//printf("brightness:%d max:%f\n", brightness, max);

	//since we already started using the bin vaiable we don't have to add the offset
	for(current_led=0;current_led<num_leds;current_led++)
	{
		/*
		 As value for an led, we use the highest value of one of its FFT frequency bins
		 (in principle, using the average may be better,
		 but if we have many bins that we sum over and that destroys the accuray)
		 the  current_led % overflow adds the missing bins by using the modulus operator
		 */
		if((current_led % overflow) == 0)
			num_bins = scaling + 1;
		else
			num_bins = scaling;
		
		//If no bin is mapped to a LED we use the data the previous LED => we don't reset max
		if(num_bins != 0)
			max = 0;
		
		for (current_bin=0;current_bin<num_bins;current_bin++)
		{
			//printf("For Led %d: %d bins current bin: %d of %d bins\n", current_led, num_bins, current_bin ,bin);
			//printf("Data[%d] = %f\n", bin, Data[bin]);
			if (Data[bin] > max)
				max = Data[bin];
			bin++;
			
		}
		//Filtering unheareble parts out
		if(max < 0.02)
			max = 0.00001;
		
		// opmization feature
		max *= amplitude_factor;
		
		// with the amplitude values between 0 and 1, we want to realize
		// visible spectrum color from 400 to 700 nm
		// But the Output between 400-450 is very dark but that is good
		//because we don't want to see low amplitudes
		amplitude = max*300.0+400;
		//write led data into struct
		(*ledstrip_data)[current_led] = spectral_color(amplitude, brightness);
		//printf("Returning: r %d, g %d, b %d\n", (*ledstrip_data)[current_led].r,(*ledstrip_data)[current_led].g,(*ledstrip_data)[current_led].b)
	}
	
}


//Displays a snake that changes it's color according to the max frequency
void mode4(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//printf("Using LED Mode 3\n");
	//cut of highs and low bins bc they are used for brightness
	int mid_bins = N/2 - high_bins- low_bins;
	//printf("midbins = %d",  mid_bins);
	static int color;
	static int current_led, current_bin, bin;
	static int max;
	
	//reset
	max = 0;
	bin = low_bins;
	
	//	for (current_bin=0;current_bin<low_bins;current_bin++)
	//	{
	//		//printf("For Bass: %d bins current bin: %d of %d bins\n", scaling + (current_led % overflow), current_bin ,bin);
	//		max += Data[bin];
	//	}
	//	//set brightness by * multipling max with 200 => the Bass adds extra brightness
	//	brightness = round(max * 200);
	//	printf("brightness:%d max:%f\n", brightness, max);
	for (current_bin=0;current_bin<mid_bins;current_bin++)
	{
		if (Data[bin] > Data[max])
			max = bin;
		bin++;
	}
	
	//copy the data from the previus time to the next led
	for(current_led=num_leds-1;current_led>0;current_led--){
		(*ledstrip_data)[current_led] = (*ledstrip_data)[current_led-1];
		//printf("For Led %d: r%d g%d b%d\n", current_led, (*ledstrip_data)[current_led].r, (*ledstrip_data)[current_led].g ,(*ledstrip_data)[current_led].b);
	}
	
	//write the new value to the first led
	//But only if the amplitude hearable
	if(max > 0.1){
		color = round((max * (1.0/(mid_bins))) * 200.0+450);
		(*ledstrip_data)[0]= spectral_color(color, 30);
	}else {
		(*ledstrip_data)[0].r = 0;
		(*ledstrip_data)[0].g = 0;
		(*ledstrip_data)[0].b = 0;
	}
}


//Displays volume as the number of LEDs and the color is the most dominant Amplitude
void mode5(unsigned long N, float * Data, int low_bins, int high_bins, int brightness, float amplitude_factor, bool logarithmic, led_data (*ledstrip_data)[]){
	//cut of highs and low bins bc they are used for brightness
	int mid_bins =(N/2) - high_bins - low_bins;
	static int color;
	static int current_led, current_bin, bin;
	static int num_leds_amp, max;
	static float max_amp, bass_max;
	
	max = 0;
	bass_max = 0;
	bin = 0;
	
	//caluclate brightness using the bass
	for (current_bin=0;current_bin<low_bins;current_bin++)
	{
		//printf("For Bass: %d bins current bin: %d bass_max=%f\n", low_bins, bin , bass_max);
		bass_max += Data[bin];
		bin++;
	}
	//set brightness by * multipling max with 200 => the Bass adds extra brightness
	brightness = round(bass_max * 200);;
	
	//get the bin with highest amplitude
	for (current_bin=0;current_bin<mid_bins;current_bin++){
		if (Data[bin] > Data[max])
			max = current_bin;
		bin++;
	}
	
	//in max whe have the bin with the maximum amplitude
	//we now want to calculate the color
	//with the max value between 0 and 1, we want to realize
	//visible spectrum color from 450 to 650 nm:
	if(max > 0.1)
		color = round((max * (1.0/(mid_bins))) * 250.0+400);
	else
		color = 400;
	
	//get the maximum amplitude
	max_amp = 1/amplitude_factor;
	
	//calculate how many LEDs are used to display
	num_leds_amp = round(num_leds * max_amp);
	//printf("max_amp=%f numleds=%d brightness=%d\n",max_amp, num_leds_amp, brightness);
	for(current_led=0;current_led<num_leds;current_led++){
		if (num_leds_amp > current_led) {
			(*ledstrip_data)[current_led]= spectral_color(color, brightness);
		}else{
			(*ledstrip_data)[current_led].r = 0;
			(*ledstrip_data)[current_led].g = 0;
			(*ledstrip_data)[current_led].b = 0;
		}
	}
}
