#define BEAT_PIN 2

#include <LiquidCrystal.h>

LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

// MIDI signals
byte MIDI_Start = 0xfa;
byte MIDI_Stop = 0xfc;
byte MIDI_Clock = 0xf8;
byte MIDI_Continue = 0xfb;

int Counter = 0;

byte Data;

void setup() {
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("Setup!");
    Serial.begin(31250);
}

void loop() {
    // Check if serial connection is available
    if(Serial.available() > 0) {

        // Read serial port
        Data = Serial.read();

        // Check for MIDI data
        if(Data == MIDI_Stop) {
            Counter = 0;
            lcd.clear();
            lcd.print(Counter);
        }
        else if(Data == MIDI_Clock) {
            ++Counter;
            lcd.clear();
            lcd.print(Counter);
        }
    }
}