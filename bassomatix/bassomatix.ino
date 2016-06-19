
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
#define PATTERN_POT A5

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


byte tick = 0;
byte  beat = 0;
int  bar = 0;
byte ticks_this_note = 0;
byte note_ptr;
byte current_note;

bool playing = false;

byte last_sync_mode = SYNC_EXT;
byte sync_mode;


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


#define MELODY_LENGTH_BARS  12
#define BASS_CHANNEL 1
//
//#define SHIFT_AMOUNT 24
const byte SHIFT_DURATIONS[] = {6, 12, 24, 48};	//#16th, 8th, quarter, half note


const byte SCALE[4][16] = {
  {0, 3, 5, 7, 10, 12, 15, 0, 3, 5, 17, 0, 19, 3, 22, 24}, //minor pentatonic
  {0, 4, 7, 7, 10, 12, 16, 0, 7, 5, 19, 0, 19, 7, 19, 24}, //major pentatonic
  {0, 0, 7, 7, 12, 12, 12, 0, 7, 12, 19, 0, 19, 7, 19, 24}, //fifths
  {0, 2, 3, 0, 2, 3, 0, 3, 3, 12, 14, 15, 12, 14, 15, 24}, //rolling blues riff
};


const byte DURATIONS[] = {24, 12, 12, 6, 6, 6, 6, 6};

#define ROOT_NOTE 48
#define CLICKS_PER_BAR (24*4)
#define TUNE_SIZE 32

byte melody[TUNE_SIZE];
byte duration[TUNE_SIZE];

byte temp_melody[TUNE_SIZE];
byte temp_duration[TUNE_SIZE];

//densitude= how often a note is playing : 0 (silent) to 255 (always note playing)
//wiggleness= variability of notes : 0= all notes the same, 8 = notes from 1 scale, 15=notes from 2 scales

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

byte read_pattern_number() {
  int raw_pattern_pot = map(analogRead(PATTERN_POT), 0, 1023, 0, 255);
  if (raw_pattern_pot > 220) {
    return 0;
  } else if (raw_pattern_pot > 180) {
    return 1;
  } else if (raw_pattern_pot > 110) {
    return 2;
  } else if (raw_pattern_pot > 55) {
    return 3;
  } else  {
    return 4; //FIXME: this should be a user scale
  }
}


byte read_densitude() {
  return map(analogRead(DENSITUDE_POT), 0, 1023, 0, 255);
}
byte read_wiggleness() {
  return map(analogRead(WIGGLENESS_POT), 0, 1023, 0, 15);
}

void generate_melody(byte root_note) {


  byte densitude = read_densitude();
  byte wiggleness = read_wiggleness();
  byte scale_number = read_scale_number();
  byte pattern_number=read_pattern_number();

  Serial.print("PATTERN ");
  Serial.print(pattern_number);
  Serial.print("SCALE ");
  Serial.print(scale_number);
  Serial.print("ROOT NOTE");
  Serial.print(root_note);
  Serial.print("DENSITUDE ");
  Serial.print(densitude);
  Serial.print("WIGGLENESS ");
  Serial.print(wiggleness);
  Serial.println("");

  byte this_duration;
  byte this_note;
  byte total_duration = 0;
  int i;
  for (i = 0; total_duration < CLICKS_PER_BAR; i++) {
    //chose a duration:
    byte this_duration = DURATIONS[random(densitude >> 5)];
    duration[i] = this_duration;
    total_duration += this_duration;
    //chose a note
    if (random(densitude + 127) < 127) {
      this_note = 0;
    } else {
      this_note = root_note + SCALE[scale_number][random(wiggleness)];
    }
    melody[i] = this_note;

    Serial.print("I=");
    Serial.print(i);
    Serial.print(" T=");
    Serial.print(total_duration);
    Serial.print(" M=");
    Serial.print(melody[i]);
    Serial.print(" D=");
    Serial.println(duration[i]);

  }

  for (; i < TUNE_SIZE; i++) {
    melody[i] = 0;
    duration[i] = 0;
  }

  ticks_this_note = 0;
  note_ptr = 0;
}


void mutate_melody() {
  byte mutation_probability = 50;
  byte densitude = read_densitude();
  byte wiggleness = read_wiggleness();
  byte scale_number = read_scale_number();
  
  int i;
  for ( i = 0; i < TUNE_SIZE; i++) {
    temp_melody[i] = melody[i];
    temp_duration[i] = duration[i];
  }


  byte min_note_length = 6;
  int j = 0;
  for (i = 0; i < TUNE_SIZE; i++) {

    if ((random(100) <= mutation_probability) && (duration[i] > min_note_length)) {
      //split this note in two
      byte duration_1 = temp_duration[i] >> 2;
      byte duration_2 = temp_duration[i] - duration_1;

      melody[j] = temp_melody[i];
      duration[j] = duration_1;
      melody[j + 1] = ROOT_NOTE + SCALE[scale_number][random(wiggleness)];;
      duration[j + 1] = duration_2;
      j += 2;
    } else {
      melody[j] = temp_melody[i];
      duration[j] = temp_duration[i];
      j++;
    }

  }
}
void start_playing() {
  Serial.println("start");


  playing = true;
  tick = 0;
  beat = 0;
  bar = 0;



  generate_melody(ROOT_NOTE) ;

}

void stop_playing() {
  Serial.println("stop");
  playing = false;

}


void do_one_tick() {
  midiSerial.write(0xF8);      //send MIDI CLOCK TICK
  tick++;
  if (tick == 24) {
    tick = 0;

    beat++;
    if (beat == 4) {
      bar++;
      beat = 0;
      ticks_this_note = 0;
      note_ptr = 0;
      if (digitalRead(HOLD_RIFF_PIN) == HIGH) {
        if (bar % 4 == 0) {
          generate_melody(ROOT_NOTE) ;
        } else {
          mutate_melody();
        }
      }
    }
  };

  if (tick == 0) {

    Serial.println("TICK 0 ");

  }

  //turn off this note on the tick  before we play the next one

  if (ticks_this_note == 1) {

    //silence last note
    if (current_note > 0) {
      midiSerial.write(0x8F + BASS_CHANNEL); //'PLAY'
      midiSerial.write(current_note);
      midiSerial.write((byte)0x00);          //velocity 0 means note off
    }
  }

  if (ticks_this_note <= 0) {
    Serial.print("TICK ");
    Serial.print(tick);
    Serial.print("NOTE PTR ");
    Serial.print(note_ptr);
    Serial.println("");


    current_note = melody[note_ptr];


    ticks_this_note = duration[note_ptr];
    Serial.print("TPN:");
    Serial.println(ticks_this_note);
    note_ptr++;
    if (current_note != 0) {
      //play new note
      midiSerial.write(0x8F + BASS_CHANNEL); //'PLAY'
      midiSerial.write(current_note);
      midiSerial.write(127);//velocity
    }
  }
  ticks_this_note--;

}



void loop() {

  unsigned long loopStart;
  unsigned long delayUntil;
  loopStart = micros();

  byte bpm = map(analogRead(A0), 0, 1023, 40, 180);
  sync_mode = digitalRead(SYNC_PIN);

  digitalWrite(LED_PIN, tick > 12 ? LOW : HIGH); //set the LED to flash once per quarter note

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
