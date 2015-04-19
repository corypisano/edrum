// Cory Pisano - edrum
//
// latest revision: January 2, 2015
// hardware: teensy 2.0

// TO DO:
//    * research multi channel adc reading, (adc queue?)
//    * oscilloscope data
//    * refactoring ( active-> listen? )
//

// peak detection algorithm ideas:
//    * continue sampling until new_sample is less than last_sample
//    * same ^ but confirm that 2 new_samples are decreasing not just 1
//    * difference between new_sample and last_sample is in interval (-inf, 5)

// sample each channel
// interleave sampling (dont sample 50 times on snare before ever sampling bass)
// on hit -> sample breaches threshold
// peak detection algorithm
// when peak, set channel to INACTIVE 
// when channel falls below threshold, place back to ACTIVE state

//           DRUM REFERENCE TABLE
//
// | index | drum    | midi | adc mux    | teensy pin
// |----------------------------------------------
// | 0     | BASS    | 36   | 0b10000010 | D7?
// | 1     | SNARE   | 38   | 0b10000011 | D7?
// 
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <usb_debug_only.h>         //DEBUG
#include <print.h>                  //DEBUG

#define THRESHOLD 20
#define NUM_INPUTS 2
#define SIZE_BUFFER 50

int ledPin = 11;

// midi notes for each drum channel
uint8_t MIDI_NOTES[] = {
  36, // bass
  38, // snare
  51  // ride
};

// ADC multiplexer values for each drum channel
uint8_t DRUM_MUXS[] = {
  0b10000010, // bass
  0b10000011, // snare
  0 // ride
};

// properties for each drum
struct drum_channel {
  uint8_t note;  // midi note for each channel (snare -> 38) 
  uint8_t mux;   // adc multiplexer value (snare -> 0b10000011)
  uint8_t sample_buffer[SIZE_BUFFER];  // data buffer
  uint8_t buffer_index;
  uint8_t velocity;   // velocity to send on playNote()
  uint8_t enabled;
};

// global drum channels 
struct drum_channel channel[NUM_INPUTS];

void setup() {

  // assign midi properties to each channel
  for (uint8_t ch = 0; ch < NUM_INPUTS; ch++) {
    channel[ch].note = MIDI_NOTES[ch];
    channel[ch].mux  = DRUM_MUXS[ch];
    channel[ch].buffer_index = 0;
    channel[ch].enabled = 1;
  }
  
  // DEBUG
  pinMode(ledPin, OUTPUT);
  delay(2000);

  initialize_adc();
  sei();
}

void loop() {
  //usbMIDI.sendNoteOn(36, 100, 1);
  
  digitalWrite(ledPin, HIGH);
  
  delay(700);
  
  usbMIDI.sendNoteOn(38, 100, 1);
  digitalWrite(ledPin, LOW);
  
  delay(700);
  
  uint8_t new_sample;
  uint8_t index;

  for (uint8_t ch = 0; ch < NUM_INPUTS ; ch++) {
    print("CHANNEL: ");
    phex(ch);
    print("         ");
    new_sample = read_ADC_channel(channel[ch].mux);

    // inactive channel processing
    if (!channel[ch].enabled) {

      // reenable channels that have settled
      if (new_sample < THRESHOLD) {
        channel[ch].enabled = 1;
      }

      continue;
    } 

    // active channel processing
    // no hit 
    if (new_sample < THRESHOLD) {
      continue; 
    }  

    // Hit detected! 
    index = channel[ch].buffer_index;              // get channel's buffer index
    channel[ch].sample_buffer[index] = new_sample; // add new sample to channel's buffer
    channel[ch].buffer_index++;                    // increment buffer index
    if (index == SIZE_BUFFER) {
      // buffer is full, play note
      channel[ch].buffer_index = 0;  // start buffer over
      playNote(ch);
    }
    else {
      channel[ch].buffer_index++;
    }
  }
  
}

// not fully implemented 
void playNote(uint8_t ch) {
  // find max value of channel's sample buffer 
  uint8_t max_velocity = THRESHOLD;
  for (int i = 0; i < SIZE_BUFFER; i++){
    if (channel[ch].sample_buffer[i] > max_velocity)  {
      max_velocity = channel[ch].sample_buffer[i];
    }
  }
  usbMIDI.sendNoteOn(channel[ch].note, max_velocity, 1);
}

void initialize_adc() {
  //AREF=AVcc
  //ADC LEFT ADJUST RESULT
  ADMUX= (1<<REFS0)|(1<<ADLAR);
  
  //ADC ENABLE, PRESCALER OF 2
  //16MHz/2=8MHz
  ADCSRA = (1<<ADEN)|(0<<ADPS2)|(0<<ADPS1)|(1<<ADPS0);
  // high speed mode?
  //ADCSRB = (1<<ADCHSM);
}

uint8_t read_ADC_channel(uint8_t ch) {
  //ch=channel 
  //ch = 0b00000011;
  ADMUX = (ADMUX & 0b11110000)|ch; //clear bottom 5 bits before ORing
  ADCSRB|=(1<<MUX5); //MUX5 bit assered for ADC 11 
  //start conversion
  ADCSRA |= (1<<ADSC);
  
  //wait for conversion to complete
  while(ADCSRA & (1<<ADSC));
  
  //ADC 10 bit value is LEFT adjusted. 8 MSBs are in ADCH.
  //Only need 7 bit resolution for MIDI data
  return ADCH>>1;
}
 
/*

*/
int16_t adc_read(uint8_t mux) {

  uint8_t low;

  ADCSRA = (1<<ADEN) | ADC_PRESCALER;  // ENable ADC
  ADCSRB = (1<<ADHSM) | (mux & 0x20);  // High Speed Mode
  ADMUX  = aref | (mux & 0x1F);        // configure mux input
  ADCSRA = (1<<ADEN) | ADC_PRESCALER | (1<<ADSC); // Start the Conversion
  //ADCSRA |= (1<<ADSC);

  while (ADCSRA & (1<<ADSC)); // wait for result
  low = ADCL;                 // must read LSB first
  return (ADCH << 8) | low;   // must read MSB only once
}