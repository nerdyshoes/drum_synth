//main synthesiser, the building blocks for the instruments


#include "synth.h"
#include "instruments.h"
#include "sequencer.h"

#include "math.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define Q15_SHIFT 15
#define Q15_ONE   (1 << Q15_SHIFT)

#define SAMPLE_RATE 48000
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_ONE (1 << FIXED_POINT_SHIFT)
#define SINE_AMPLITUDE (1 << 15) // Q1.15 format

#define ADSR_WAVETABLE_SIZE 1024

static int16_t sine_table[SINE_TABLE_SIZE];




ADSR envelope;

Oscillator osc;  // Define the variables
ADSR amp_env;
ADSR pitch_env;
Filter hpf;


ADSR amp_env_snare;
ADSR pitch_env_snare;
Oscillator osc_snare;

Filter hpf_hihat;
ADSR amp_env_hihat;



#include "main.h"  // Ensures HAL and peripheral declarations are included

extern UART_HandleTypeDef huart2;  // Declares that huart2 is defined elsewhere


int16_t white_noise() {
    return (rand() % 65535) - 32767;
}
void filter_init(Filter* filter, FilterPassType type, int16_t cutoff_frequency, float q) {
	float f_float = 2.0f * sinf(M_PI * cutoff_frequency / SAMPLE_RATE); // normalized
	filter->f = (int16_t)(f_float * Q15_ONE);
	filter->q = (int16_t)(q * Q15_ONE);
	filter->type = type;

}

int16_t filter_process(Filter* filter, int16_t sample_in) {
    // Use int32_t for intermediate values to avoid overflow
    int32_t high = (int32_t)sample_in - filter->low - ((int32_t)filter->q * filter->band >> Q15_SHIFT);
    int32_t band = (int32_t)filter->band + ((int32_t)filter->f * high >> Q15_SHIFT);
    int32_t low  = (int32_t)filter->low  + ((int32_t)filter->f * band >> Q15_SHIFT);

    // Saturate to Q15 range
    if (band > INT16_MAX) band = INT16_MAX;
    else if (band < INT16_MIN) band = INT16_MIN;

    if (low > INT16_MAX) low = INT16_MAX;
    else if (low < INT16_MIN) low = INT16_MIN;

    // Update internal state
    filter->band = (int16_t)band;
    filter->low = (int16_t)low;

    // Return the selected output
    switch (filter->type) {
        case LOW:
            return filter->low;
        case HIGH:
            return (int16_t)high;
        case BANDPASS:
            return filter->band;
        default:
            return 0;
    }

}


void generate_sine_table() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        sine_table[i] = (int16_t)(sinf(angle) * SINE_AMPLITUDE);
    }

}

int16_t waveform_sine(uint32_t phase) {
	uint16_t index = (phase >> 22) & (SINE_TABLE_SIZE - 1);
//	printf("Phase: %lu, Index: %u, Value: %d \r\n", phase, index, sine_table[index]);
    return sine_table[index];
}

void print_sine_table() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        printf("sine_table[%d] = %d\r\n", i, sine_table[i]);
    }
}

int16_t oscillator_sine_next_sample(Oscillator* osc, int16_t mod_value) {
	int16_t true_mod_value = (int32_t)mod_value * osc->mod_percentage >> 15;
    int32_t modulated_frequency = osc->frequency + ((osc->frequency * true_mod_value) >> 15);
    uint32_t phase_increment = ((uint64_t)modulated_frequency * UINT32_MAX) / SAMPLE_RATE;
    osc->phase += phase_increment;
//    printf("%d", waveform_sine(osc->phase));
    return waveform_sine(osc->phase);
}

int16_t vca(int16_t input, int16_t gain) {
    return (int16_t)(((int32_t)input * gain) >> 15);
}



void fill_attack(ADSR* env) {
//	printf("hi\r\n");
	float base = 1 - 1/env->scale;

    for (uint32_t i=0; i<ADSR_WAVETABLE_SIZE; i++) {
    	float curve = env->A * env->scale * (1 - powf(base, (float)i/(float)ADSR_WAVETABLE_SIZE));
        env->attack_wavetable[i] = (int16_t)curve;
    }
//    printf("bye\r\n");
}

int16_t get_attack(ADSR* env, float attack_progress) {
	int16_t index = (int16_t)(attack_progress * ADSR_WAVETABLE_SIZE);

	return env->attack_wavetable[index];
}

void print_attack(ADSR* env) {
	for (uint32_t i=0; i<ADSR_WAVETABLE_SIZE; i++) {
		printf("%i \r\n", env->attack_wavetable[i]);
	}
}

void fill_decay(ADSR* env) {

	float base = 1 - 1/env->scale;

    for (uint32_t i=0; i<ADSR_WAVETABLE_SIZE; i++) {
    	float curve = env->A * (env->scale * (1 - pow(base, (float)i/(float)ADSR_WAVETABLE_SIZE)) + 1 - env->scale);
        env->decay_wavetable[i] = (int16_t)curve;
    }
}

int16_t get_decay(ADSR* env, float attack_progress) {
	int16_t index = (int16_t)(attack_progress * ADSR_WAVETABLE_SIZE);
	int16_t table_value =  env->decay_wavetable[index];
	return ((int32_t)(table_value * (32767 - env->s_int16)) >> 15 ) + ((int32_t)(32767*env->s_int16) >> 15);
}

void fill_release(ADSR* env) {

	float base = 1 - 1/env->scale;

    for (uint32_t i=0; i<ADSR_WAVETABLE_SIZE; i++) {
    	float curve = env->A * (env->scale * (1 - pow(base, (float)i/(float)ADSR_WAVETABLE_SIZE)) - env->scale + 1);
        env->release_wavetable[i] = (int16_t)curve;
    }
}

int16_t get_release(ADSR* env, float attack_progress) {
	int16_t index = (int16_t)(attack_progress * ADSR_WAVETABLE_SIZE);

	return (int32_t)(env->release_wavetable[index] * env->s_int16) >> 15;
}

void adsr_init(ADSR* env, double a_time, double d_time, double s_level, double r_time) {
    env->attack_time = a_time; // 1 millisecond
    env->decay_time = d_time;
    env->s = s_level;   //from 0-1, sustain level
    env->release_time = r_time;
    env->s_int16 = (int16_t)(env->s*32767);

    env->max_sustain_time = 0.001;

    env->attack_samples = (uint32_t)(env->attack_time * SAMPLE_RATE);
    env->decay_samples = (uint32_t)(env->decay_time * SAMPLE_RATE);
    env->max_sustain_samples = (uint32_t)(env->max_sustain_time * SAMPLE_RATE);
    env->release_samples = (uint32_t)(env->release_time * SAMPLE_RATE);

    env->sustain_level = (int16_t)(env->s * 32767);


    env->timer = 0;
    env->gain = 0;
    env->state = ATTACK;

    env->scale = 1.05;
    env->a = -log(1 - 1/env->scale) / env->attack_time;
    env->A = 32767;
    env->s_lower = env->s - env->scale + 1;
    env->d = -log((env->scale - 1)/(1 - env->s_lower)) / env->decay_time;
    env->r = -log(1 - 1/env->scale)/env->release_time;



//    printf("hello\r\n");
    fill_attack(env);
    fill_decay(env);
    fill_release(env);

//    print_attack(env);


}




 int16_t adsr_process(ADSR* env) {
     switch (env->state) {
         case ATTACK:

        	 env->gain = get_attack(env, (float)env->timer / (float)env->attack_samples);
//        	 printf("Attack gain: %i\r\n", )
             env->timer += 1;

             if (env->timer >= env->attack_samples) {
                 env->state = DECAY;
                 env->timer = 0;
             }

             break;

         case DECAY:
             // printf("DECAY\n");
        	 env->gain = get_decay(env, (float)env->timer / (float)env->attack_samples);
        	 env->timer += 1;

             if (env->timer >= env->decay_samples) {
                 env->state = SUSTAIN;
                 env->timer = 0;
             }

             break;

         case SUSTAIN:
             // printf("SUSTAIN\n");
             env->gain = env->sustain_level;
             env->timer += 1;

             if (env->timer >= env->max_sustain_samples) {
                 env->state = RELEASE;
                 env->timer = 0;
             }

             break;

         case RELEASE:
             // printf("RELEASE\n");
        	 env->gain = get_release(env, (float)env->timer / (float)env->attack_samples);
        	 env->timer += 1;

             if (env->timer >= env->release_samples) {
                 env->state = IDLE;
                 env->timer = 0;
             }

             break;

         case IDLE:
             env->gain = 0;
     }


     return env->gain;
 }



void adsr_trigger(ADSR* env) {
    env->state = ATTACK;
    env->gain = 0;
    env->timer = 0;
}

void adsr_release(ADSR* env) {
    if (env->state != IDLE) {
        env->state = RELEASE;
        env->timer = 0;
    }
}










int16_t setup() {
	printf("setup starting\n");



    generate_sine_table();


    adsr_init(&amp_env, 0.006, 0.005, 0, 0.0072);


    adsr_init(&pitch_env, 0.005, 0.0816, 0, 0.007);




    sequencer_init();

    adsr_init(&amp_env_snare, 0.01217, 0.001, 0, 0.010);

    setup_instruments();

    filter_init(&hpf, HIGH, 1062, 0.707f);

//    print_sine_table();


    adsr_init(&amp_env_hihat, 0.001, 0.001, 0, 0.005);
    filter_init(&hpf_hihat, HIGH, 3258, 0.707f);




    printf("setup complete.\n");
    return 1;
}




