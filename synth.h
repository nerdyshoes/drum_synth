#ifndef SYNTH_H
#define SYNTH_H
#include <stdint.h>
#include <stdbool.h>
#define SINE_TABLE_SIZE 1024
#define ADSR_WAVETABLE_SIZE 1024



// Oscillator structure
typedef struct Oscillator {
    uint32_t phase;       // Phase accumulator
    int16_t frequency;   // Frequency in fixed-point format
    int16_t mod_percentage;
} Oscillator;


typedef enum { ATTACK, DECAY, SUSTAIN, RELEASE, IDLE } EnvelopeState;
typedef enum {LOW, HIGH, BANDPASS} FilterPassType;

typedef struct {
	int16_t low;
	int16_t band;
	int16_t f;
	int16_t q;
	FilterPassType type;
} Filter;

typedef struct {
    EnvelopeState state;
    int16_t gain;
    float attack_time;
    uint32_t attack_samples;
    float decay_time;
    uint32_t decay_samples;
    float max_sustain_time;
    uint32_t max_sustain_samples;
    float release_time;
    uint32_t release_samples;
    int16_t sustain_level;
    float s;
    int16_t s_int16;
    uint32_t timer;

//    int16_t *attack_wavetable;
//    int16_t *decay_wavetable;
//    int16_t *release_wavetable;

    float scale;
    float a;
    float A;
    float d;
    float r;
    float s_lower;

    float attack_pow_small[1024];
    float attack_pow_large[1024];

    int16_t attack_wavetable[ADSR_WAVETABLE_SIZE];
    int16_t decay_wavetable[ADSR_WAVETABLE_SIZE];
    int16_t release_wavetable[ADSR_WAVETABLE_SIZE];
} ADSR;

typedef struct {
    int16_t prev_output;
    int16_t prev_input;
} LowPassFilter;






extern ADSR envelope;

extern Oscillator osc;  // Define the variables
extern ADSR amp_env;
extern ADSR pitch_env;
extern Filter hpf;


extern ADSR amp_env_snare;
extern ADSR pitch_env_snare;
extern Oscillator osc_snare;

extern Filter hpf_hihat;
extern ADSR amp_env_hihat;

extern Filter bpf_clap;
extern ADSR amp_env_clap;
extern ADSR filter_env_clap;

extern Filter bpf_cowbell;
extern ADSR amp_env_cowbell;
extern Oscillator osc_cowbell_1;
extern Oscillator osc_cowbell_2;





int16_t setup();
void adsr_trigger(ADSR* env);
void adsr_release(ADSR* env);
int16_t adsr_process(ADSR* env);
int16_t oscillator_sine_next_sample(Oscillator* osc, int16_t mod_value);
int16_t white_noise();
int16_t filter_process(Filter* filter, int16_t sample_in);
int16_t filter_process_freqmod(Filter* filter, int16_t sample_in, int16_t f);
#endif // SYNTH_H
