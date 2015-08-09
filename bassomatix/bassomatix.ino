
#include <AltSoftSerial.h>


/*
Bass-O-Matx mk I
https://github.com/jonnosan/bassomatix/blob/master/README.md


Circuit is set up as:

D10 (switch) : hold current riff
D11 (LED) : beat clock
D12 (sitch) : sync: external MIDI or internal
A0 (10K pot) : tempo (when running internal clock)
A2 (10K pot) : densitude (low = more silence)
A3 (10K pot) : wiggleness (high = more variation in selected notes)
A4 (5 notch voltage divider) : Scale (TBD...)
A5 (5 notch voltage divider) : Rhythm Type (TBD..)

 */

AltSoftSerial midiSerial;

#define LED_PIN 11
#define SYNC_MODE 12
#define SYNC_EXT 0
#define SYNC_INT 1




void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  //  Set MIDI baud rate:
  midiSerial.begin(31250);

  pinMode(10, INPUT_PULLUP);  //hold riff
  pinMode(12, INPUT_PULLUP);  //sync mode
  pinMode(11, OUTPUT); //LED - beat clock
  digitalWrite(LED_PIN, HIGH);
}


byte tick = 0;

byte last_sync_mode = SYNC_EXT;
byte sync_mode;

void loop() {

  unsigned long loopStart;
  unsigned long delayUntil;
  loopStart = micros();

  byte bpm = map(analogRead(A0), 0, 1023, 40, 180);
  sync_mode = digitalRead(SYNC_MODE);

  digitalWrite(LED_PIN, tick > 12 ? LOW : HIGH); //set the LED to flash once per quarter note

  if (sync_mode != last_sync_mode) {
    delay(100); //to debounce
    if (sync_mode == SYNC_INT) {
      midiSerial.write(0xFA);      //changing from EXT to INT => START playing
      tick = 0;

      Serial.println("start");

    } else {
      midiSerial.write(0xFC);      //changing from INT to EXT => STOP playing
      Serial.println("stop");
    }
  }


  last_sync_mode = sync_mode;




  if (sync_mode == SYNC_INT) {
    midiSerial.write(0xF8);      //MIDI CLOCK TICK

    delayUntil = loopStart + (60000000L / (24L * bpm));

    //burn cycles until we are ready for the next click
    for (; micros() < delayUntil;) {}

    
  } else { //if we are in external sync mode, poll for a clock tick, or SYNC mode changes
 
    for (bool done = false; !done;) {
      if (midiSerial.available()) {
        byte c = midiSerial.read();
        if (c == 0xF8) {
          done = true;
        }
      }
      if (digitalRead(SYNC_MODE) == SYNC_INT) {
        done = true;
      }
    }
  }
  tick++;
  if (tick == 24) {
    tick = 0;
  };

}
