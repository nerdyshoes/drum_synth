// where you build your instruments out of the parts of the synthesiser



#include "synth.h"
#include "stm32f4xx_hal.h"




void kick_init(Oscillator* osc, ADSR* amp_env, ADSR* pitch_env) {

    osc->phase = 0;
    osc->frequency = 60; // Initial frequency
    osc->mod_percentage = 32767;

    adsr_trigger(amp_env);
    adsr_trigger(pitch_env);


}
void cowbell_init(Oscillator* osc1, Oscillator* osc2, ADSR* amp_env) {

    osc1->phase = 0;
    osc1->frequency = 587; // Initial frequency
    osc1->mod_percentage = 0;

    osc2->phase = 0;
    osc2->frequency = 845; // Initial frequency
    osc2->mod_percentage = 0;

    adsr_trigger(amp_env);



}
void snare_init(ADSR* amp_env) {

    adsr_trigger(amp_env);

}


void hihat_init(ADSR* amp_env) {
	adsr_trigger(amp_env);
}








int16_t kick_sample(int16_t gate_output, int16_t trigger_output) {

	int16_t sample = 0;

	// if trigger, initialise the kick drum
	if (trigger_output > 0) {
		//reset the kickdrum
		osc.phase = 0;
		//    osc->frequency = 60; // Initial frequency
		adsr_trigger(&amp_env);
		adsr_trigger(&pitch_env);
	}

	else if (trigger_output < 0) {
		// if trig < 0 then the button has been released
	    adsr_release(&amp_env);
	    adsr_release(&pitch_env);
	}

	//process drum if not idle
	//check to see if the amplitude adsr is in a state that is not 0
	//process drum if not in idle
	if (amp_env.state != IDLE) {

	    // Process amplitude envelope
		int16_t amp_gain = adsr_process(&amp_env);

	    // Process pitch envelope (use it as frequency modulation amount)
	    int16_t fm_amount = adsr_process(&pitch_env);

	    // Generate oscillator sample with frequency modulation
	    int16_t osc_sample = oscillator_sine_next_sample(&osc, fm_amount);

	    sample =  (int16_t)(((int32_t)osc_sample * (int32_t)amp_gain) >> 15);
	}


	return sample;
}

int16_t snare_sample(int16_t gate_output, int16_t trigger_output){

	int16_t sample = 0;
	// if trigger, initialise the snare drum
	if (trigger_output > 0) {
		adsr_trigger(&amp_env_snare);
	}
	else if (trigger_output < 0) {
		adsr_release(&amp_env_snare);
	}
	if (amp_env_snare.state != IDLE) {

		int16_t amp_gain = adsr_process(&amp_env_snare);

		int16_t wn_sample = white_noise();
		int16_t gain_sample = ((int32_t)(amp_gain * wn_sample) >> 15);
	//	printf("amp_gain: %d\n", amp_gain);
		int16_t filtered_noise = filter_process(&hpf, gain_sample);

		sample = filtered_noise;

	}

	return sample;
}

int16_t hihat_sample(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;

	if (trigger_output > 0) {
		adsr_trigger(&amp_env_hihat);
	}
	else if (trigger_output < 0) {
		adsr_release(&amp_env_hihat);
	}
	if (amp_env_hihat.state != IDLE) {
		int16_t amp_gain = adsr_process(&amp_env_hihat);

		int16_t wn_sample = white_noise();

		int16_t gain_sample = ((int32_t)(amp_gain * wn_sample) >> 15);

		int16_t filtered_noise = filter_process(&hpf_hihat, gain_sample);

//		sample = pn_sample;
		sample = filtered_noise;
	}

	return sample;
}


int16_t clap_sample(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;

	if (trigger_output > 0) {
		adsr_trigger(&amp_env_clap);
		adsr_trigger(&filter_env_clap);
	}
	else if (trigger_output < 0) {
		adsr_release(&amp_env_clap);
		adsr_release(&filter_env_clap);
	}
	if (amp_env_clap.state != IDLE) {
		int16_t amp_gain = adsr_process(&amp_env_clap);
		int16_t filter_mod = adsr_process(&filter_env_clap);

		int16_t wn_sample = white_noise();

		int16_t gain_sample = ((int32_t)(amp_gain * wn_sample) >> 15);

		int16_t filtered_noise = filter_process_freqmod(&bpf_clap, gain_sample, filter_mod);

//		sample = pn_sample;
		sample = filtered_noise;
	}

	return sample;
}
int16_t cowbell_sample(int16_t gate_output, int16_t trigger_output) {
	int16_t sample = 0;

	if (trigger_output > 0) {
		osc_cowbell_1.phase = 0;
		osc_cowbell_2.phase = 0;
		adsr_trigger(&amp_env_cowbell);
	}
	else if (trigger_output < 0) {
		adsr_release(&amp_env_cowbell);
	}
	if (amp_env_cowbell.state != IDLE) {
		int16_t amp_gain = adsr_process(&amp_env_cowbell);

		int16_t oscillator1 = oscillator_sine_next_sample(&osc_cowbell_1, 0);
		int16_t oscillator2 = oscillator_sine_next_sample(&osc_cowbell_2, 0);

		int16_t mixed = (oscillator1 >> 1) + (oscillator2 >> 1);

		int16_t gain_sample = ((int32_t)(amp_gain * mixed) >> 15);

		int16_t filtered = filter_process(&bpf_clap, gain_sample);

		sample = filtered;
	}

	return sample;
}
void setup_instruments() {
	kick_init(&osc, &amp_env, &pitch_env);
	snare_init(&amp_env_snare);
	hihat_init(&amp_env_hihat);
	cowbell_init(&osc_cowbell_1, &osc_cowbell_2, &amp_env_cowbell);
}
