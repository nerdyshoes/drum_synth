// start oscillator at 60 hz, pitch spikes up then decays
// attack 1 ms, 20ms decay roughly









#include "synth.h"
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

int8_t bool_setup = 0;

ADSR envelope;

static Oscillator modulator = { .phase = 0, .frequency = 1, .mod_percentage=32767 };
static Oscillator carrier;
Oscillator osc;  // Define the variables
ADSR amp_env;
ADSR pitch_env;
Filter hpf;


ADSR amp_env_snare;
ADSR pitch_env_snare;
Oscillator osc_snare;

Filter hpf_hihat;
ADSR amp_env_hihat;


Sequencer seq;

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
	bool_setup = 1;
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

void kick_init(Oscillator* osc, ADSR* amp_env, ADSR* pitch_env) {

    osc->phase = 0;
    osc->frequency = 60; // Initial frequency
    osc->mod_percentage = 32767;

    adsr_trigger(amp_env);
    adsr_trigger(pitch_env);


}
void kick_trigger(Oscillator* osc, ADSR* amp_env, ADSR* pitch_env) {
    osc->phase = 0;
    osc->frequency = 60; // Initial frequency

    adsr_trigger(amp_env);
    adsr_trigger(pitch_env);

}
void kick_release(Oscillator* osc, ADSR* amp_env, ADSR* pitch_env) {

    adsr_release(amp_env);
    adsr_release(pitch_env);

}

int16_t kick_process(Oscillator* osc, ADSR* amp_env, ADSR* pitch_env) {
    // Process amplitude envelope
    int16_t amp_gain = adsr_process(amp_env);


    // Process pitch envelope (use it as frequency modulation amount)
    int16_t fm_amount = adsr_process(pitch_env);

    // Generate oscillator sample with frequency modulation
    int16_t sample = oscillator_sine_next_sample(osc, fm_amount);

    // Apply amplitude envelope
//    printf("amp gain:%i\r\n", amp_gain);
//    if (amp_gain != 0) {
//    	char buffer[50];
//		snprintf(buffer, sizeof(buffer), "amp gain:%i\r\n", amp_gain);
//		HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 10);
//    }


//    return amp_gain;
    return (int16_t)(((int32_t)sample * (int32_t)amp_gain) >> 15);


    // return sample;
    // return amp_gain;
    // return fm_amount;
}



void snare_init(ADSR* amp_env) {

    adsr_trigger(amp_env);


}
void snare_trigger(ADSR* amp_env) {

    adsr_trigger(amp_env);


}
void snare_release(ADSR* amp_env) {

    adsr_release(amp_env);

}

int16_t snare_process(ADSR* amp_env, Filter* hpf) {
	int16_t amp_gain = adsr_process(amp_env);

	int16_t sample = white_noise();
	int16_t gain_sample = ((int32_t)(amp_gain * sample) >> 15);
//	printf("amp_gain: %d\n", amp_gain);
	int16_t filtered_noise = filter_process(hpf, gain_sample);
    return filtered_noise;
}



int16_t hihat_process(ADSR* amp_env, Filter* hpf) {
	int16_t amp_gain = adsr_process(amp_env);

	int16_t sample = white_noise();

	int16_t gain_sample = ((int32_t)(amp_gain * sample) >> 15);

	int16_t filtered_noise = filter_process(hpf, gain_sample);

    return filtered_noise;
}


void hihat_init(ADSR* amp_env) {
	adsr_trigger(amp_env);
}

void hihat_trigger(ADSR* amp_env) {
    adsr_trigger(amp_env);
}

void hihat_release(ADSR* amp_env) {

    adsr_release(amp_env);

}






#define SEQUENCER_STEPS 8


void sequencer_init(Sequencer* seq, double bpm) {

	if (bpm == 0) {
	    printf("Error: BPM must be greater than 0!\n");
	    return; // Exit function early
	}
    double seconds_per_beat = 60/bpm;
    seq->samples_per_beat = (int32_t)(SAMPLE_RATE * seconds_per_beat);

    seq->current_step = 0;
    seq->gate_output = 0;
    seq->trigger_output = 0;
    seq->timer = 0;
    seq->gate_length = (int16_t)(32767 * 0.75); //75% of the full beat the gate is on
    seq->gate_samples = (seq->samples_per_beat * (int32_t)seq->gate_length) >> 15;


}


void sequencer_tick(Sequencer* seq) {
    // returns seq->current_step, seq->gate_output, seq->trigger_output
//	printf("seq->timer: %li, seq->gate_samples: %li\n", seq->timer, seq->gate_samples);
	if (seq->samples_per_beat == 0) {
		return;
	}
//	printf("samples_per_beat: %li \n", seq->samples_per_beat);
//	printf("gate_samples: %li\n", seq->gate_samples);
//	printf("timer: %ld, gate_samples: %ld\r\n", seq->timer, seq->gate_samples);
	if (seq->timer == 0){
	    seq->trigger_output = 32767;
	    seq->gate_output = 32767;
	}
	else if (seq->timer < seq->gate_samples) {
//		printf("hewwo\n");
		seq->gate_output = 32767;
		seq->trigger_output = 0;
	}
    else if (seq->timer == seq->gate_samples) {
        seq->trigger_output = -32767;
        seq->gate_output = 0;
    }
    else if (seq->timer >= seq->samples_per_beat) {

        seq->current_step = (seq->current_step + 1)%SEQUENCER_STEPS;
        seq->timer = 0;
        return;
    }
    else if (seq->timer > seq->gate_samples) {
        seq->trigger_output = 0;
    }




    seq->timer++;
}







int16_t snare_drum_sample(int16_t gate_output, int16_t trigger_output){
	int16_t sample = 0;
	// if trigger, initialise the kick drum
	if (trigger_output > 0) {
		snare_trigger(&amp_env_snare);
	}
	else if (trigger_output < 0) {
		snare_release(&amp_env_snare);
	}
	if (gate_output > 0) {
		sample = snare_process(&amp_env_snare, &hpf);
	}

	return sample;
}



int16_t kick_drum_sample(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	// if trigger, initialise the kick drum
	if (trigger_output > 0) {
		kick_trigger(&osc, &amp_env, &pitch_env);
	}
	else if (trigger_output < 0) {
		kick_release(&osc, &amp_env, &pitch_env);
	}
	if (gate_output > 0) {
		sample = kick_process(&osc, &amp_env, &pitch_env);
	}

	return sample;
}

int16_t hihat_sample(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;

	if (trigger_output > 0) {
		hihat_trigger(&amp_env_hihat);
	}
	else if (trigger_output < 0) {
		hihat_release(&amp_env_hihat);
	}
	if (gate_output > 0) {
		sample = hihat_process(&amp_env_hihat, &hpf_hihat);
	}
	return sample;
}


int16_t step_0(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = kick_drum_sample(gate_output, trigger_output);
//	sample += snare_drum_sample(gate_output, trigger_output) >> 2;
	return sample;
}

int16_t step_1(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = kick_drum_sample(gate_output, trigger_output);
//	sample += snare_drum_sample(gate_output, trigger_output) >> 2;
//	sample += hihat_sample(gate_output, trigger_output) >> 2;
	return sample;
}

int16_t step_2(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = hihat_sample(gate_output, trigger_output);
	return sample;
}

int16_t step_3(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = kick_drum_sample(gate_output, trigger_output);


//	sample = kick_drum_sample(gate_output, trigger_output);
//	sample = ((int32_t)gate_output * (int32_t)oscillator_sine_next_sample(&carrier, 0)) >> 15;
//	sample = white_noise();
//	sample = snare_drum_sample(gate_output, trigger_output) >> 1;

//	sample += hihat_sample(gate_output, trigger_output) >> 2;
	return sample;
}
int16_t step_4(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = kick_drum_sample(gate_output, trigger_output);
//	sample += hihat_sample(gate_output, trigger_output) >> 2;
	return sample;
}
int16_t step_5(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = kick_drum_sample(gate_output, trigger_output);
	return sample;
}
int16_t step_6(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = snare_drum_sample(gate_output, trigger_output);
//	sample += hihat_sample(gate_output, trigger_output) >> 2;

	return sample;
}
int16_t step_7(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;
	sample = snare_drum_sample(gate_output, trigger_output) >> 1;
	return sample;
}



void setup() {
	printf("setup starting\n");



    generate_sine_table();


    adsr_init(&amp_env, 0.006, 0.005, 0, 0.0072);


    adsr_init(&pitch_env, 0.005, 0.0816, 0, 0.007);




    sequencer_init(&seq, 200);

    adsr_init(&amp_env_snare, 0.01217, 0.001, 0, 0.010);

    kick_init(&osc, &amp_env, &pitch_env);
    snare_init(&amp_env_snare);

    filter_init(&hpf, HIGH, 1062, 0.707f);

//    print_sine_table();


    adsr_init(&amp_env_hihat, 0.001, 0.001, 0, 0.005);
    filter_init(&hpf_hihat, HIGH, 3258, 0.707f);




    printf("setup complete.\n");

    carrier.phase = 0;
    carrier.frequency = 400;
    bool_setup = 2;


}



int16_t generate_sample(int16_t *adc_values, int16_t adc_values_size) {
    if (bool_setup != 2) {
//        printf("Warning: sequencer2_init() not run yet!\n");
        return 0;  // Prevent uninitialized access
    }


    int16_t volume = adc_values[3] << 3;


//    int16_t bass_pitch_mod = (int16_t)(adc_values[0] / 4095.0 * 70 + 50);
//    osc.frequency = bass_pitch_mod;
//    osc.frequency = 80;






	int16_t sample = 0;
	sequencer_tick(&seq);


//	sequencer
	switch (seq.current_step) {
		case 0:
			sample = step_0(seq.gate_output, seq.trigger_output);
			break;

		case 1:
			sample = step_1(seq.gate_output, seq.trigger_output);
			break;

		case 2:
			sample = step_2(seq.gate_output, seq.trigger_output);
			break;

		case 3:
			sample = step_3(seq.gate_output, seq.trigger_output);
			break;
		case 4:
			sample = step_4(seq.gate_output, seq.trigger_output);
			break;
		case 5:
			sample = step_5(seq.gate_output, seq.trigger_output);
			break;
		case 6:
			sample = step_6(seq.gate_output, seq.trigger_output);
			break;
		case 7:
			sample = step_7(seq.gate_output, seq.trigger_output);
			break;
	}







//	if (sample != 0) {
//		printf("sample: %d, current_step: %d\r\n", sample, seq.current_step);
//	}

	//mixer


//	sample = oscillator_sine_next_sample(&osc, 0);
//	sample = 0;
	return sample;
//	return (int32_t)(sample*volume) >> 15;
}
