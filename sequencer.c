#include "synth.h"
#include <stdio.h>
#include "instruments.h"
#include "sequencer.h"

#define SEQUENCER_STEPS 8
#define SAMPLE_RATE 48000

Sequencer seq;

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


int16_t generate_sample(int16_t *adc_values, int16_t adc_values_size) {


//    int16_t volume = adc_values[3] << 3;


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








	//mixer


//	sample = oscillator_sine_next_sample(&osc, 0);
//	printf("sample: %d\r\n", sample);
//	if (sample != 0) {
//		printf("sample: %d\r\n", sample);
//	}

//	sample = 0;
	return sample;
//	return (int32_t)(sample*volume) >> 15;
}
