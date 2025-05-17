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


int16_t generate_sample(int16_t *adcValues, int16_t adcValues_size);
void sequencer_init();
