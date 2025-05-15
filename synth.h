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

#define SEQUENCER_STEPS 8


typedef struct {
    uint8_t current_step;
    int16_t gate_output;
    int16_t trigger_output;
    uint32_t timer;
    int32_t samples_per_beat;
    int16_t gate_length; //a percentage from 0-32767
    int32_t gate_samples;
} Sequencer;


extern Oscillator osc;  // Declare external variables
extern ADSR amp_env;
extern ADSR pitch_env;


int16_t generate_sample(int16_t *adcValues, int16_t adcValues_size);
void setup();
#endif // SYNTH_H
