/*
edrum test
 
converting to interrupt based
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#define THRESHOLD 20
#define NUM_INPUTS 2
#define BUFF_SIZE 16
#define BUFF_SIZE_MASK (BUFF_SIZE - 1)

#define SNARE 0
#define BASS 1



// circular buffer, each drum has one
typedef struct circ_buffer {
  volatile uint8_t buff[BUFF_SIZE];
  volatile uint8_t writeIndex;
  volatile uint8_t readIndex;
}

// properties for each drum
typedef struct drum {
  uint8_t note;       // midi note for each drum (snare -> 38) 
  uint8_t mux;        // adc multiplexer value (snare -> 0b10000011)
  uint8_t velocity;   // velocity to send on playNote()
  uint8_t enabled;    // timer based enable/disable

  circ_buffer buffer;  // data buffer
  // buffer should be last in struct for efficient memory packing
}

/*
write to buffer
*/
void write(circ_buffer* buffer, uint8_t value) {
  buffer->buff[buffer->writeIndex & BUFF_SIZE_MASK] = value;
  writeIndex++;
}

/*
read from buffer at position
*/
uint8_t getSample(circ_buffer* buffer, uint8_t pos) {
  return buffer->buff[(buffer->readIndex + pos) & BUFF_SIZE_MASK];
  readIndex++;
};

/*
return how many samples are in the buffer
*/
uint8_t numBufferedSamples(circ_buffer* buffer) {
  return (writeIndex - readIndex ) % BUFF_SIZE;
}

// global drum channels 
struct drum_channel drum[NUM_INPUTS];

void setup() {

  // Snare drum setup
  drum[SNARE].note = 38;
  drum[SNARE].mux  = 1;

  // Bass drum setup
  drum[BASS].note = 36;
  drum[BASS].mux  = 4;

  // assign midi properties to each drum
  for (uint8_t ch = 0; ch < NUM_INPUTS; ch++) {
    drum[ch].buffer_index = 0;
    drum[ch].enabled      = 1;
  }
  
  initialize_adc();
  sei();
}

void loop() {
  uint8_t new_sample;
  uint8_t index;

  // for loop needed?
  // switch to ch++ > NUM_INPUTS
  for (uint8_t ch = 0; ch < NUM_INPUTS ; ch++) {


    sample = getSample(drum[ch].buff, 0);

    // inactive drum processing
    if (!drum[ch].enabled) {

      // reenable drums that have settled
      if (new_sample < THRESHOLD) {
        drum[ch].enabled = 1;
      }

      continue;
    } 

    // active drum processing
    // no hit 
    if (sample < THRESHOLD) {
      continue; 
    }  

    // Hit detected! 

    if (numBufferedSamples(drum[ch].buffer) < MINIMUM_SAMPLES) {
      continue;
    }

    if (detectPeak(drum[ch].buffer)) {
      velocity = drum[ch].peak;
      playNote(ch, );
    }
  }
  
}

/*
playNote
*/
void playNote(uint8_t ch, uint8_t velocity) {
  usbMIDI.sendNoteOn(drum[ch].note, velocity, 1);
}

/*

*/
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

/*

*/
void adc_start(uint8_t mux, uint8_t aref) {

  ADCSRA = (1<<ADEN) | ADC_PRESCALER;  // ENable ADC, interrupt disabled
  ADCSRB = (1<<ADHSM) | (mux & 0x20);  // High Speed Mode
  ADMUX  = aref | (mux & 0x1F);        // configure mux and ref

  head = 0;
  tail = 0;

  ADCSRA = (1<<ADSC) | (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | ADC_PRESCALER;

  sei();  // enable interrupts
}

ISR(ADC_vect) {

  static uint8_t val;
  static uint8_t buffalo_wing;

  // read current drum, write to buffer
  analog_read  = ADCH;
  write(drum[current_drum].buff, analog_read);

  current_drum++;
  if (current_drum) > NUM_INPUTS {
    current_drum = 0;
  }  

  ADMUX = channel[current_drum].mux;
  ADCSRA |= 1<<ADSC; // start next conversion

  }
}

