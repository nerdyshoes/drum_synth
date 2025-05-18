// where you build your instruments out of the parts of the synthesiser



#include "synth.h"





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


int16_t snare_sample(int16_t gate_output, int16_t trigger_output){
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



int16_t kick_sample(int16_t gate_output, int16_t trigger_output) {
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

void setup_instruments() {
	kick_init(&osc, &amp_env, &pitch_env);
	snare_init(&amp_env_snare);
}
