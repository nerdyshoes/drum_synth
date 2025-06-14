#include "synth.h"
#include <stdio.h>
#include "instruments.h"
#include "sequencer.h"
#include "main.h"
#include <math.h>

#define SEQUENCER_STEPS 8
#define SAMPLE_RATE 48000

Sequencer seq;
Button button;
Button button1;
Button button2;
Button button3;
Button button4;
Button button5;

void sequencer_init() {

	double bpm = 200;
    double seconds_per_beat = 60/bpm;
    seq.samples_per_beat = (int32_t)(SAMPLE_RATE * seconds_per_beat);

    seq.current_step = 0;
    seq.gate_output = 0;
    seq.trigger_output = 0;
    seq.timer = 0;
    seq.gate_length = (int16_t)(32767 * 0.75); //75% of the full beat the gate is on
    seq.gate_samples = (seq.samples_per_beat * (int32_t)seq.gate_length) >> 15;

    button.prev_state = 0;
    button1.prev_state = 0;
    button2.prev_state = 0;
    button3.prev_state = 0;
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



int16_t sequencer_generate() {
	int16_t sample = 0;
	switch (seq.current_step) {
		case 0:
			sample = kick_sample(seq.gate_output, seq.trigger_output);
			break;

		case 1:
			sample = snare_sample(seq.gate_output, seq.trigger_output);
			break;

		case 2:
			sample = hihat_sample(seq.gate_output, seq.trigger_output);
			break;

		case 3:
			sample = kick_sample(seq.gate_output, seq.trigger_output);
			break;
		case 4:
			sample = kick_sample(seq.gate_output, seq.trigger_output);
			break;
		case 5:
			sample = hihat_sample(seq.gate_output, seq.trigger_output);
			break;
		case 6:
			sample = snare_sample(seq.gate_output, seq.trigger_output);
			break;
		case 7:
			sample = kick_sample(seq.gate_output, seq.trigger_output);
			break;
	}
	return sample;
}


int32_t synth_from_button() {

	int32_t sample = 0;
	int16_t gate = 0;
	int16_t gate_kick = 0;
	int16_t gate_snare = 0;
	int16_t gate_hihat = 0;
	int16_t gate_clap = 0;
	int16_t gate_cowbell = 0;
	int16_t trigger = 0;
	int16_t trigger_kick = 0;
	int16_t trigger_snare = 0;
	int16_t trigger_hihat = 0;
	int16_t trigger_clap = 0;
	int16_t trigger_cowbell = 0;


	// onboard button
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
	    // Button is pressed (logic 0)

		gate = 32767;
		if (button.prev_state == 0) {
			trigger = 32767;
		}
	}
	else {
	    // Button is not pressed (logic 1)

	}


	// button 1, kick
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET) {
	    // Button is pressed (logic 0)

		gate_kick = 32767;
		if (button1.prev_state == 0) {
			trigger_kick = 32767;
		}
	}
	else {
	    // Button is not pressed (logic 1)

		if (button1.prev_state > 0) {
			//if button was on last tick, but off now
			trigger_kick = -32767;
		}
	}


	// button 2, snare
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) {

	    // Button is pressed (logic 0)
		gate_snare = 32767;
		if (button2.prev_state == 0) {

			trigger_snare = 32767;
		}
	}
	else {
	    // Button is not pressed (logic 1)

		if (button2.prev_state > 0) {

			//if button was on last tick, but off now
			trigger_snare = -32767;
		}
	}

	//button 3, hihat
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_RESET) {
	    // Button is pressed (logic 0)

		gate_hihat = 32767;
		if (button3.prev_state == 0) {
//			printf("hello\n");
			trigger_hihat = 32767;
		}
	}
	else {
	    // Button is not pressed (logic 1)
		if (button3.prev_state > 0) {
			//if button was on last tick, but off now
			trigger_hihat = -32767;
		}
	}

	// button 4, clap
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET) {
	    // Button is pressed (logic 0)

		gate_clap = 32767;
		if (button4.prev_state == 0) {
			trigger_clap = 32767;
		}
	}
	else {
	    // Button is not pressed (logic 1)
		if (button4.prev_state > 0) {
			//if button was on last tick, but off now
			trigger_clap = -32767;
		}
	}

	//button 5, cowbell
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_RESET) {
	    // Button is pressed (logic 0)

		gate_cowbell = 32767;
		if (button5.prev_state == 0) {
			trigger_cowbell = 32767;
		}
	}
	else {
	    // Button is not pressed (logic 1)
		if (button5.prev_state > 0) {
			//if button was on last tick, but off now
			trigger_cowbell = -32767;
		}
	}


	sample = kick_sample(gate_kick, trigger_kick) >> 1;
	sample += snare_sample(gate_snare, trigger_snare) >> 1;
	sample += hihat_sample(gate_hihat, trigger_hihat) >> 1;
	sample += clap_sample(gate_clap, trigger_clap) >> 1;
	sample += cowbell_sample(gate_cowbell, trigger_cowbell) >> 1;



	button.prev_state = gate;
	button1.prev_state = gate_kick;
	button2.prev_state = gate_snare;
	button3.prev_state = gate_hihat;
	button4.prev_state = gate_clap;
	button5.prev_state = gate_cowbell;
	return sample;
}

void knob_1(int16_t value) {
// value is 12 bit, from 0-4095


// scales to range of 80, and adds 40 to get between 40-120Hz
	int16_t new_frequency = ((value * 80) / 4095) + 40;


// frequency range between 40-120 Hz
	osc.frequency = new_frequency;

//		int32_t new_attack_samples = ((int32_t)(value * 23952) / 4095) + 48;
//		int32_t new_attack_samples = exp((float)value / 300.0f) + 40;
//		if (new_attack_samples > 32767) new_attack_samples = 32767;
//		else if (new_attack_samples < 0) new_attack_samples = 0;
//	amp_env_cowbell.attack_samples = new_attack_samples;
}

void knob_2(int16_t value) {

	// we want from 0.001s to 10s ideally, try to start with 0.001 - 0.5s
	// therefore in samples we want between 48-24000
	// so a range of 23952

	int32_t new_attack_samples = ((int32_t)(value * 23952) / 4095) + 48;
	int32_t new_attack_samples = exp((float)value / 300.0f) + 40;
	if (new_attack_samples > 32767) new_attack_samples = 32767;
	else if (new_attack_samples < 0) new_attack_samples = 0;
	amp_env.attack_samples = new_attack_samples;

//	int32_t new_decay_samples = exp((float)value / 300.0f) + 40;
//	if (new_decay_samples > 32767) new_decay_samples = 32767;
//	else if (new_decay_samples < 0) new_decay_samples = 0;
//	amp_env_cowbell.decay_samples = new_decay_samples;
}

void knob_3(int16_t value) {

	// we want from 0.001s to 10s ideally, try to start with 0.001 - 0.5s
	// therefore in samples we want between 48-24000
	// so a range of 23952

	int32_t new_decay_samples = exp((float)value / 300.0f) + 40;
	if (new_decay_samples > 32767) new_decay_samples = 32767;
	else if (new_decay_samples < 0) new_decay_samples = 0;
	amp_env.decay_samples = new_decay_samples;

//	int32_t new_sustain_level = ((int32_t)(value * 23952) / 4095) + 48;
//	amp_env_cowbell.sustain_level = new_sustain_level;
}


void apply_knobs(int16_t *adc_values, int16_t adc_values_size) {
	knob_1(adc_values[0]);
	knob_2(adc_values[1]);
	knob_3(adc_values[2]);
}



int16_t generate_sample(int16_t *adc_values, int16_t adc_values_size) {


//    int16_t volume = adc_values[3] << 3;


//    int16_t bass_pitch_mod = (int16_t)(adc_values[0] / 4095.0 * 70 + 50);
//    osc.frequency = bass_pitch_mod;
//    osc.frequency = 80;



//	printf("%d, %d, %d\n", adc_values[0],adc_values[1],adc_values[2]);


	int32_t sample = 0;



	apply_knobs(adc_values, adc_values_size);

//	sequencer_tick(&seq);
//	sample = sequencer_generate();



	sample = synth_from_button();
	int16_t sample_16bit;
	if (sample > 32767) sample_16bit = 32767;
	else if (sample < -32767) sample_16bit = -32767;
	else sample_16bit = (int16_t)sample;


//	sample = oscillator_sine_next_sample(&osc, 0);
//	printf("sample: %ld\r\n", sample);
//	if (sample != 0) {
//		printf("sample: %d\r\n", sample);
//	}

//	sample = 0;
//	printf("%d, %ld\n", sample_16bit, sample);
	return sample_16bit;
//	return (int32_t)(sample*volume) >> 15;
}
