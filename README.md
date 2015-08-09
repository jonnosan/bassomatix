# Bass-O-Matix - Mark I

Arduino bassline generator

Always uses MIDI channel 1

Circuit is set up as:

- D8           : MIDI RX
- D9           : MIDI TX
- D10 (switch) : hold current riff
- D11 (LED)    : beat clock
- D12 (sitch)  : sync: external MIDI or internal
- A0 (10K pot) : tempo (when running internal clock)
- A2 (10K pot) : densitude    (low = more silence)
- A3 (10K pot) : wiggleness   (high = more variation in selected notes)
- A4 (5 notch voltage divider) : Scale (TBD...)
- A5 (5 notch voltage divider) : Rhythm Type (TBD..)



