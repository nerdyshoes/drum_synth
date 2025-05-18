#include "synth.h"
#include <stdio.h>
#include "instruments.h"
#include "sequencer.h"
#include "main.h"

#define SEQUENCER_STEPS 8
#define SAMPLE_RATE 48000

Sequencer seq;
Button button;
Button button1;
Button button2;
Button button3;

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
	int16_t gate;
	int16_t gate_kick;
	int16_t gate_snare;
	int16_t gate_hihat;
	int16_t trigger = 0;
	int16_t trigger_kick = 0;
	int16_t trigger_snare = 0;
	int16_t trigger_hihat = 0;


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
		gate = 0;
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
		gate_kick = 0;
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
		gate_snare = 0;
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
		gate_hihat = 0;
	}

	sample = kick_sample(gate_kick, trigger_kick) >> 1;
	sample += snare_sample(gate_snare, trigger_snare) >> 1;
	sample += hihat_sample(gate_hihat, trigger_hihat) >> 1;



	button.prev_state = gate;
	button1.prev_state = gate_kick;
	button2.prev_state = gate_snare;
	button3.prev_state = gate_hihat;
	return sample;
}


int16_t generate_sample(int16_t *adc_values, int16_t adc_values_size) {


//    int16_t volume = adc_values[3] << 3;


//    int16_t bass_pitch_mod = (int16_t)(adc_values[0] / 4095.0 * 70 + 50);
//    osc.frequency = bass_pitch_mod;
//    osc.frequency = 80;



//	printf("%d, %d, %d\n", adc_values[0],adc_values[1],adc_values[2]);


	int32_t sample = 0;
//	sequencer_tick(&seq);
//	sample = sequencer_generate();
	sample = synth_from_button();
	int16_t sample_16bit;
	if (sample > 32767) sample_16bit = 32767;
	else if (sample < -32767) sample_16bit = -32767;


//	sample = oscillator_sine_next_sample(&osc, 0);
//	printf("sample: %d\r\n", sample);
//	if (sample != 0) {
//		printf("sample: %d\r\n", sample);
//	}

//	sample = 0;
	return sample_16bit;
//	return (int32_t)(sample*volume) >> 15;
}
