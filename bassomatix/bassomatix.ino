
#include <AltSoftSerial.h>


/*
Bass-O-Matx mk I
https://github.com/jonnosan/bassomatix/blob/master/README.md


Circuit is set up as:

D8: MIDI RX
D9: MIDI TX
D10 (switch) : hold current riff
D11 (LED) : beat clock
D12 (sitch) : sync: external MIDI or internal
A0 (10K pot) : tempo (when running internal clock)
A3 (10K pot) : densitude (low = more silence)
A1 (10K pot) : wiggleness (high = more variation in selected notes)
A4 (5 notch voltage divider) : Scale (TBD...)

A5 (5 notch voltage divider) : :Pattern Type 


 */

AltSoftSerial midiSerial;

#define LED_PIN 11
#define SYNC_EXT 0
#define SYNC_INT 1
#define SYNC_PIN 12
#define HOLD_RIFF_PIN 10

#define TEMPO_POT A0
#define DENSITUDE_POT A3
#define WIGGLENESS_POT A1
#define SCALE_POT A4
#define RIFFLENGTH_POT A5

#define STEPS_PER_BAR 16
#define MAX_STEPS_PER_RIFF (8*STEPS_PER_BAR)


void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  //  Set MIDI baud rate:
  midiSerial.begin(31250);

  pinMode(HOLD_RIFF_PIN, INPUT_PULLUP);  //hold riff
  pinMode(SYNC_PIN, INPUT_PULLUP);  //sync mode
  pinMode(LED_PIN, OUTPUT); //LED - beat clock
  digitalWrite(LED_PIN, HIGH);


}


byte riff[MAX_STEPS_PER_RIFF];
byte tick = 0;
byte  beat_in_bar = 0;
byte  beat_in_riff = 0;
byte bars_in_riff=4;
int  bar = 0;
byte last_note=0;

bool playing = false;

byte last_sync_mode = SYNC_EXT;
byte sync_mode;

#define BASS_CHANNEL 1



//called for each byte received on MIDI IN
//echo (almost) everything to OUT (aka 'soft THRU')
void process_midi_byte(byte b) {

  //don't echo MIDI clock
  if (b == 0XF8) {
    return;
  }

  if (b == 0XFA) { // MIDI song start
    start_playing();
    Serial.print("got START");
  }


  if (b == 0XFC) { // MIDI song stop
    stop_playing();
    Serial.print("got STOP");
  }

  midiSerial.write(b);
}


const byte SCALE[4][16] = {
  {0, 3, 5, 7, 10, 12, 15, 0, 3, 5, 17, 0, 19, 3, 22, 24}, //minor pentatonic
  {0, 4, 7, 7, 10, 12, 16, 0, 7, 5, 19, 0, 19, 7, 19, 24}, //major pentatonic
  {0, 0, 7, 7, 12, 12, 12, 0, 7, 12, 19, 0, 19, 7, 19, 24}, //fifths
  {0, 2, 3, 0, 2, 3, 0, 3, 3, 12, 14, 15, 12, 14, 15, 24}, //rolling blues riff
};



#define ROOT_NOTE 48
//#define CLICKS_PER_BAR (24*4)

byte read_scale_number() {
  int raw_scale_pot = map(analogRead(SCALE_POT), 0, 1023, 0, 255);
  if (raw_scale_pot > 220) {
    return 0;
  } else if (raw_scale_pot > 180) {
    return 1;
  } else if (raw_scale_pot > 110) {
    return 2;
  } else if (raw_scale_pot > 55) {
    return 3;
  } else  {
    return 0; //FIXME: this should be a user scale
  }
}

byte read_riff_length() {
  int raw_RIFFLENGTH_POT = map(analogRead(RIFFLENGTH_POT), 0, 1023, 0, 255);
  if (raw_RIFFLENGTH_POT > 220) {
    return 1;
  } else if (raw_RIFFLENGTH_POT > 180) {
    return 2;
  } else if (raw_RIFFLENGTH_POT > 110) {
    return 3;
  } else if (raw_RIFFLENGTH_POT > 55) {
    return 4;
  } else  {
    return 8; //FIXME: this should be a user scale
  }
}


byte read_densitude() {
  return map(analogRead(DENSITUDE_POT), 0, 1023, 0, 15);
}
byte read_wiggleness() {
  return map(analogRead(WIGGLENESS_POT), 0, 1023, 1, 15);
}


void generate_riff() {
  
  
  byte dens=read_densitude();
  byte wiggle=read_wiggleness();
  byte scale=read_scale_number();
  byte i = dens%4;
  bars_in_riff=read_riff_length();
 int steps_in_riff=bars_in_riff*STEPS_PER_BAR;

  Serial.print("BAR: ");      
  Serial.print( bar);
  Serial.print(" OF ");      
  Serial.print( bars_in_riff);



  byte pulses=bars_in_riff*(1+(dens>>1));

  for (i = 0; i < (MAX_STEPS_PER_RIFF); i++) {
    riff[i] = 0;
  }

  i = 0;

  byte pauses  = steps_in_riff - pulses;
  if (pulses >= 0) {
    int per_pulse = (pauses / pulses);
    int remainder = pauses % pulses;

    for (byte pulse = 0; pulse < pulses; pulse++) {
      riff[i % steps_in_riff] = ROOT_NOTE+SCALE[scale][pulse%wiggle];
      i += 1;
      i += per_pulse;
      if (pulse < remainder) {
        i++;
      }
    }
  }
  
  
}
void start_playing() {
  Serial.println("start");


  playing = true;
  tick = 0;
  beat_in_bar = 0;
  beat_in_riff = 0;  
  bar = 0;



  generate_riff() ;

}

void stop_playing() {
  Serial.println("stop");
  playing = false;

}


void do_one_tick() {
  midiSerial.write(0xF8);      //send MIDI CLOCK TICK
  if (tick%6 == 0) {
    tick = 0;
    
    if(last_note>0) {
      midiSerial.write(0x8F + BASS_CHANNEL); //'PLAY'
      midiSerial.write(last_note);
      midiSerial.write(byte(0));//velocity
    }


    if(riff[beat_in_riff]>0) {
      last_note=riff[beat_in_riff];
      midiSerial.write(0x8F + BASS_CHANNEL); //'PLAY'
      midiSerial.write(last_note);
      midiSerial.write(127);//velocity
    }

    beat_in_bar++;
    beat_in_riff++;
    
    if (beat_in_bar == STEPS_PER_BAR) {
      
      bar++;
      beat_in_bar = 0;      
      generate_riff();
      if (bar>=bars_in_riff) {
        beat_in_riff=0;
        bar=0;
      }
    }
  };
  tick++;


}



void loop() {

  unsigned long loopStart;
  unsigned long delayUntil;
  loopStart = micros();

  byte bpm = map(analogRead(A0), 0, 1023, 40, 180);
  sync_mode = digitalRead(SYNC_PIN);

  digitalWrite(LED_PIN, beat_in_bar%4 == 0  ? LOW : HIGH); //set the LED to flash once per quarter note

  if (sync_mode != last_sync_mode) {
    delay(100); //to debounce
    if (sync_mode == SYNC_INT) {
      midiSerial.write(0xFA);      //changing from EXT to INT => START playing
      start_playing();

    } else {
      midiSerial.write(0xFC);      //changing from INT to EXT => STOP playing
      stop_playing();
    }
    last_sync_mode = sync_mode;
  }




  if (sync_mode == SYNC_INT) {

    delayUntil = loopStart + (60000000L / (24L * bpm));

    //burn cycles until we are ready for the next click
    for (; micros() < delayUntil;) {
      if (midiSerial.available()) {
        byte c = midiSerial.read();
        process_midi_byte(c);
      }
    }


  } else { //if we are in external sync mode, poll for a clock tick, or SYNC mode changes

    for (bool done = false; !done;) {
      if (midiSerial.available()) {
        byte c = midiSerial.read();
        if (c == 0xF8) {
          done = true;
        }
        process_midi_byte(c);
      }
      if (digitalRead(SYNC_PIN) == SYNC_INT) {
        done = true;
      }
    }
  }

  do_one_tick();


}
