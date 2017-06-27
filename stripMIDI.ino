#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Message.h>
#include <midi_Namespace.h>
#include <midi_Settings.h>

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3

boolean updateFlag = false;

byte commandByte;
byte noteByte;
byte velocityByte;
byte LEDNote;

const byte noteOn = 144;
const byte noteOff = 128;
const byte pitchBend = 224;

int colorGroup;
int pPin;
int sPin;
int tPin;
int pColor;
int sColor;
int bend;

void setup()
{
    pinMode(REDPIN, OUTPUT);
    pinMode(GREENPIN, OUTPUT);
    pinMode(BLUEPIN, OUTPUT);
    MIDI.begin(1);
    Serial.begin(115200);
}

//collect incoming MIDI data
void checkMIDI(){
  if (Serial.available() > 2){
    commandByte = Serial.read();//read first byte
    noteByte = Serial.read();//read next byte
    velocityByte = Serial.read();//read final byte 
    updateFlag = true;
  }
}

//calculate lighting parameters from MIDI data
void setLightParams(){
  LEDNote = noteByte;
  colorGroup = LEDNote/42 + 1;
  //use note to specify primary, secondary, and tertiary color
  if (colorGroup == 1){
    pPin = REDPIN;
    sPin = GREENPIN;
    tPin = BLUEPIN;
  }
  else if (colorGroup == 2){
    pPin = GREENPIN;
    sPin = BLUEPIN;
    tPin = REDPIN;
  }
  else{
    pPin = BLUEPIN;
    sPin = REDPIN;
    tPin = GREENPIN;
  }
  //use note to calculate mix of primary and secondary color
  pColor = 255 - (noteByte%42*255/41);
  sColor = 255 - pColor;
  //use velocity to scale intensity
  pColor = pColor*velocityByte/127;
  sColor = sColor*velocityByte/127;
}

//turn on primary and secondary colors
void lightsOn(){
  analogWrite(pPin, pColor);
  analogWrite(sPin, sColor);
  analogWrite(tPin, 0);
}

//turn off all colors
void lightsOff(){
  analogWrite(pPin, 0);
  analogWrite(sPin, 0);
  analogWrite(tPin, 0);
  pColor = 0;
  sColor = 0;
}

//fade in tertiary color
void fadeIn(){
  analogWrite(pPin, pColor);
  analogWrite(sPin, sColor);
  analogWrite(tPin, (bend - 65)*255/64);
}

//fade out primary and secondary colors
//by continuity of analog pitchbend control, tertiary color should be off
void fadeOut(){
  analogWrite(pPin, pColor*bend/64);
  analogWrite(sPin, sColor*bend/64);
  analogWrite(tPin, 0);
}

//update LED strip
void updateLights(){
  //if new noteOn received, update lights for new note
  if (commandByte == noteOn && velocityByte > 0){
    setLightParams();
    lightsOn();
  }
  //if noteOff is received for the CURRENT note, turn off lights
  //comparison is by note, so fades/bends ignored
  else if ((commandByte == noteOff || velocityByte == 0) && noteByte == LEDNote){
    lightsOff();
  }
  //if pitch is bent up, fade in tColor; if bent down, fade out pColor & sColor
  else if (commandByte == pitchBend){
    bend = noteByte*256 | velocityByte;
    if (bend > 64){
      fadeIn();
    }
    else if(bend < 64){
      fadeOut();
    }
  }
  updateFlag = false;
}

void loop()
{
    checkMIDI();
    if (updateFlag == true){
      updateLights();  
    }
}
