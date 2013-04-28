/*
Cory Pisano, 2013

**change from 'playSnare()' and 'acd_read(snarePin)'
to: playNote(note) and adc_read(pin)

*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <MIDI.h>

#define THRESHOLD 12
#define SNARE 38
#define BASS 36
#define snarePin 0b10000011    //fix to basic 8 bit val, and in adc_read: convert
#define bassPin 0b10000010

//long val;
uint16_t adc_result;

void setup() {
  delay(3000);
  MIDI.begin();
  Serial.begin(57600);  //set baud rate
  adc_init();
  sei();  //enable interrupts
}

void loop() 
{
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
  ADCSRB = (1<<ADHSM);
}

uint16_t adc_read(uint8_t ch)
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
 
