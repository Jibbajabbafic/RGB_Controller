// Including library code
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

byte midi_start = 0xfa;
byte midi_stop = 0xfc;
byte midi_clock = 0xf8;
byte midi_continue = 0xfb;
int play_flag = 0;
byte data;
byte counter;
bool reset = 0;

unsigned long interval;
unsigned long prev_millis = 0;
unsigned long now_millis = 0;
unsigned long prev_interval_signal = 0;
unsigned long reset_interval = 3000;

void setup() {
    lcd.begin(16, 2);
    Serial1.begin(31250);
    lcd.clear();
    lcd.print("Setup!");
}

void loop() {
    if(Serial1.available() > 0) {
        if (reset) {
            reset = 0;
            lcd.clear();
        }

        data = Serial1.read();
        // if(data == midi_start) {
        //     play_flag = 1;
        // }
        // else if(data == midi_continue) {
        //     play_flag = 1;
        // }
        //else 
        if(data == midi_stop) {
            // play_flag = 0;
            counter = 0;
            analogWrite(31, 0);
        }
        else if((data == midi_clock) /*&& (play_flag == 1)*/) {
            Sync();
        }
        
        prev_interval_signal = millis();
    }
    else if (millis() - prev_interval_signal >= reset_interval && !reset) {
        reset = 1;
        play_flag = 0;
        counter = 0;
        lcd.clear();
        lcd.print("No clock signal!");
    }
}

void Sync() {
    if (counter == 23) {
        counter = 0;
        prev_millis = now_millis;
        now_millis = millis();
        interval = now_millis - prev_millis;

        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print(60000.0/interval);
        lcd.print(" BPM");
        analogWrite(31, 255);
    }
    else {
        ++counter;
    }

    if (counter == 3) {
        analogWrite(31, 0);
    }
    
    lcd.setCursor(0,0);
    lcd.print(counter);
}