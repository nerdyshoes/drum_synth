# drum_synth
A digital synthesiser that makes drum sounds. Designed for the stm32f446re, but should work with anything that runs c and has a dac.

main.c handles all the background stuff like reading and writing to output.

synth.c is the meat of the synthesiser. It is where all the components like ADSRs and oscillators are defined.

instruments.c is where you construct your synthesiser out of the synth components, like kick drum sounds.

sequencer.c is where you sequence notes by either interacting with the real world or by sequence.
