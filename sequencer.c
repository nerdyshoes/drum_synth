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

int32_t sequence[2048] = {0};

#define MAX_EVENTS 512
uint8_t  seq_instr[MAX_EVENTS];   // 0 = no sound, 1 = kick, 2 = snare, etc.
uint32_t seq_len   [MAX_EVENTS];   // length of that event in samples


uint16_t set_bit(uint16_t val, uint8_t pos, int bit) {
    if (pos > 15) return val; // Out-of-range bit positions are ignored
    if (bit)
        return val | (1 << pos);     // Set bit at position `pos`
    else
        return val & ~(1 << pos);    // Clear bit at position `pos`
}

uint8_t read_bit(uint32_t val, uint8_t pos) {
    if (pos > 31) return 0;  // prevent undefined behavior
    return (val >> pos) & 1;
}


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

void read_buttons(uint16_t *button_state, int16_t size) {


	button_state[0] = set_bit(button_state[0], 0, !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6));
	button_state[1] = set_bit(button_state[1], 1, !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7));
	button_state[2] = set_bit(button_state[2], 2, !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
	button_state[3] = set_bit(button_state[3], 3, !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8));

}


event_count = 0;
int32_t synth_from_button() {



	static uint16_t count = 0;
	static uint16_t seq_index = 0;
	uint16_t button_state[4] = {0};

	int8_t currently_recording = 0;
	if (count == 200) {


		count = 0;
		seq_index += 1;

		if (seq_index == 2048) {
			seq_index = 0;
		}

		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == 0) {
			// recording, BUTTON iS ON
			currently_recording = 1;
		}

	}



	read_buttons(button_state, 4);

	int16_t gate_kick_seq = read_bit(sequence[seq_index], 0) * 32767;
	int16_t gate_snare_seq = read_bit(sequence[seq_index], 1) * 32767;
	int16_t gate_hihat_seq = read_bit(sequence[seq_index], 2) * 32767;
	int16_t gate_clap_seq = read_bit(sequence[seq_index], 3) * 32767;

	int16_t trigger_kick_seq = 0;
	int16_t trigger_snare_seq = 0;
	int16_t trigger_hihat_seq = 0;
	int16_t trigger_clap_seq = 0;

	int16_t prev_seq_index;
	if (seq_index == 0) {
		prev_seq_index = 2047;
	}
	else {
		prev_seq_index = seq_index - 1;
	}


	if (count == 0) {
		//only trigger when count=0 to ensure no multiple triggers
		if (gate_kick_seq && !read_bit(sequence[prev_seq_index], 0)) {
			trigger_kick_seq = 32767;
		}
		else if (!gate_kick_seq && read_bit(sequence[prev_seq_index], 0)) {
			trigger_kick_seq = -32767;
		}

		if (gate_snare_seq && !read_bit(sequence[prev_seq_index], 1)) {
			trigger_snare_seq = 32767;
		}
		else if (!gate_snare_seq && read_bit(sequence[prev_seq_index], 1)) {
			trigger_snare_seq = -32767;
		}

		if (gate_hihat_seq && !read_bit(sequence[prev_seq_index], 2)) {
			trigger_hihat_seq = 32767;
		}
		else if (!gate_hihat_seq && read_bit(sequence[prev_seq_index], 2)) {
			trigger_hihat_seq = -32767;
		}

		if (gate_clap_seq && !read_bit(sequence[prev_seq_index], 3)) {
			trigger_clap_seq = 32767;
		}
		else if (!gate_clap_seq && read_bit(sequence[prev_seq_index], 3)) {
			trigger_clap_seq = -32767;
		}
	}


	int32_t sample = 0;


	int16_t gate_kick = 0;
	int16_t gate_snare = 0;
	int16_t gate_hihat = 0;
	int16_t gate_clap = 0;

	int16_t trigger_kick = 0;
	int16_t trigger_snare = 0;
	int16_t trigger_hihat = 0;
	int16_t trigger_clap = 0;




		// button 1, kick
		if (button_state[0]) {
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
		if (button_state[1]) {

		    // Button is pressed (logic 0)
			gate_snare = 32767;
			sequence[seq_index] = set_bit(sequence[seq_index], 1, 1);
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
		if (button_state[2]) {
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
		if (button_state[3]) {
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



	sample = kick_sample(gate_kick, trigger_kick);
	sample += kick_sample(gate_kick_seq, trigger_kick_seq);

	sample += snare_sample(gate_snare, trigger_snare) >> 2;
	sample += snare_sample(gate_snare_seq, trigger_snare_seq) >> 2;

	sample += hihat_sample(gate_hihat, trigger_hihat) >> 1;
	sample += hihat_sample(gate_hihat_seq, trigger_hihat_seq) >> 1;

	sample += clap_sample(gate_clap, trigger_clap) >> 1;
	sample += clap_sample(gate_clap_seq, trigger_clap_seq) >> 1;



	button1.prev_state = gate_kick;
	button2.prev_state = gate_snare;
	button3.prev_state = gate_hihat;
	button4.prev_state = gate_clap;

	if (currently_recording) {

		sequence[seq_index] = set_bit(sequence[seq_index], 0, gate_kick>0);
		sequence[seq_index] = set_bit(sequence[seq_index], 1, gate_snare>0);
		sequence[seq_index] = set_bit(sequence[seq_index], 2, gate_hihat>0);
		sequence[seq_index] = set_bit(sequence[seq_index], 3, gate_clap>0);
	}
	count++;
	return sample;
}

void knob_1(int16_t value) {
// value is 12 bit, from 0-4095


// scales to range of 80, and adds 40 to get between 20-120Hz
	int16_t new_frequency = ((value * 100) / 4095) + 20;


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

//	int32_t new_attack_samples = ((int32_t)(value * 23952) / 4095) + 48;
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
