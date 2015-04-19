/*
* Cory Pisano
*
* March 5, 2014 thought- 
*   enum {SNARE, BASS, TOM, HIHAT, ...} for use of "channel[SNARE]"
*   
*/

/* HEADERS */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <MIDI.h>

/* GLOBALS */
#define THRESHOLD 20
#define NCHANNELS 2
#define SIZE_BUFFER 50

/* to be removed */
#define SNARE 38
#define BASS 36
#define snarePin 0b10000011    //fix to basic 8 bit val, and in adc_read: convert
#define bassPin 0b10000010

struct midi_channel {
    uint8_t note;             // midi note for each channel (snare -> 38) 
    uint8_t adc_mux;          // adc multiplexer value (snare -> 0b10000011)
    uint8_t sample_buffer[SIZE_BUFFER];     // data buffer
    uint8_t buffer_index;
    uint8_t velocity;         // velocity to send on playNote()
};

/* global midi channels */
struct midi_channel channel[NCHANNELS];


/* initialize midi channels, initialize MIDI and serial communication, enable interrupts */
void setup() {
 
  /* SNARE CHANNEL */
  channel[0].note = 38;
  channel[0].adc_mux = 0b10000011;

  /* BASS CHANNEL */
  channel[1].note = 36;
  channel[1].adc_mux = 0b10000010;

  /* initialize midi channel buffers to 0's */
  for (uint8_t ch = 0; ch < NCHANNELS; ch++) {
    //channel[ch].sample_buffer = {0};
    channel[ch].buffer_index = 0;
  }
  
  delay(3000); // needed ?
  MIDI.begin();
  Serial.begin(57600);  //set baud rate
  adc_init();
  sei();  //enable interrupts
}

void loop() 
{
  uint8_t new_sample;
  uint8_t index;

  for (uint8_t ch = 0; ch < NCHANNELS; ch++) {

    new_sample = adc_read(channel[ch].adc_mux);

    /* no hit */
    if (new_sample < THRESHOLD) {
      continue; // on to the next one, on to the next one, ...
    }  

    /* Hit detected! */
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

  /*
  //check snare
  adc_result = adc_read(snarePin);
  if (adc_result>THRESHOLD)
  {
    playSnare();
  } 
  //check bass
  adc_result = adc_read(bassPin);
  if (adc_result>THRESHOLD)
  {
    playBass();
  } 
  */
}

/* not fully implemented */
void playNote(uint8_t ch)
{
  /* find max value of channel's sample buffer */
  uint8_t max_velocity = THRESHOLD;
  for (int i = 0; i < SIZE_BUFFER; i++){
    if (channel[ch].sample_buffer[i] > max_velocity)  {
      max_velocity = channel[ch].sample_buffer[i];
    }
  }
  MIDI.sendNoteOn(channel[ch].note, max_velocity, 1);
}

void playBass()
{
  int valueNew=0;
  int valuePeak=adc_read(bassPin);
  for (long sample=0;sample<75;sample++)
  {
    valueNew = adc_read(bassPin);
    if (valueNew>valuePeak)
    {
      valuePeak=valueNew;
    }
  }
  MIDI.sendNoteOn(BASS,valuePeak,1);
  delay(30);
}  

void playSnare()
{
  int valueNew=0;
  int valuePeak=adc_read(snarePin);
  for (long sample=0;sample<75;sample++)
  {
    valueNew = adc_read(snarePin);
    if (valueNew>valuePeak)
    {
      valuePeak=valueNew;
    }
  }
  MIDI.sendNoteOn(SNARE,valuePeak,1);
  delay(40);
}

void adc_init()
{
  //AREF=AVcc
  //ADC LEFT ADJUST RESULT
  ADMUX= (1<<REFS0)|(1<<ADLAR);
  
  //ADC ENABLE, PRESCALER OF 2
  //16MHz/2=8MHz
  ADCSRA = (1<<ADEN)|(0<<ADPS2)|(0<<ADPS1)|(1<<ADPS0);
  // high speed mode?
  //ADCSRB = (1<<ADCHSM);
}

uint8_t adc_read(uint8_t ch)
{
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
 
