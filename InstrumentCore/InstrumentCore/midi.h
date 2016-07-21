/*
 * midi.h
 *
 * Created: 2016-07-21 10:10:38 AM
 *  Author: vwfchung
 */ 

#ifndef MIDI_H_
#define MIDI_H_

#include <avr/io.h>
#include <util/setbaud.h>

void midi_init();
void midi_set_channel(uint8_t channel);

void midi_send_noteon(uint8_t note, uint8_t velocity);
void midi_send_noteoff(uint8_t note, uint8_t velocity);
void midi_send_programchange(uint8_t program);


#endif /* MIDI_H_ */