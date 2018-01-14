// Define pins for inputs
#define INC_PIN 20
#define DEC_PIN 21

// Define pins for outputs
#define R_PIN 13
#define G_PIN 12
#define B_PIN 11

// Debounce time for inputs
#define DEBOUNCE 100

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// GLOBAL VARIABLES
byte BPM = 120;

class LED {
    public:
        // Byte array for current RGB values
        byte RGB[3] = {0, 0, 0};
        // Types of pattern
        enum pattern { NONE, FLASH, FADE };
        
        // VARIABLES
        pattern ActivePattern;
        // pattern NextPattern;

        // Variables for timings
        unsigned long onTime;
        unsigned long offTime;
        unsigned long TotalInterval;
        unsigned long Interval;
        unsigned long lastUpdate;
        byte colourNo = 0;
        byte prevBPM = 0;
        byte Multiplier = 1;
        float dutyCycle;
        bool onState;

        void updateTiming() {
            TotalInterval = BPMtoMS(BPM*Multiplier);
            onTime = TotalInterval*dutyCycle;
            offTime = TotalInterval - onTime;
        }

        void Update() {
            if(prevBPM != BPM) {
                prevBPM = BPM;
                updateTiming();
            }

            if((millis() - lastUpdate) > Interval) {
                lastUpdate = millis();
                switch(ActivePattern) {
                case FLASH:
                    FlashUpdate();
                    break;
                case FADE:
                    // FadeUpdate();
                    break;
                default:
                    break;
                }
            }
        }

        void Flash(float duty = 0.25, byte mult = 1) {
            ActivePattern = FLASH;
            Multiplier = mult > 0 ? mult : 1;
            TotalInterval = BPMtoMS(BPM*Multiplier);
            dutyCycle = duty;
            onTime = TotalInterval*dutyCycle;
            offTime = TotalInterval - onTime;
            Interval = onTime;
            colourNo = 0;
            onState = false;
        }

        void FlashUpdate() {
            // Check if lights are in ON or OFF state
            if(onState) {
                RGB[0] = 0;
                RGB[1] = 0;
                RGB[2] = 0;
                Interval = offTime;
                onState = false;
            }
            else {
                // Select the next colour
                if(colourNo >= 2) {
                colourNo = 0;
                }
                else {
                colourNo++;
                }

                RGB[colourNo] = 255;
                Interval = onTime;
                onState = true;
            }

            // Set the lights
            setColour(RGB[0], RGB[1], RGB[2]);  
        }

        void setColour(byte red, byte green, byte blue) {
            analogWrite(R_PIN, red);
            analogWrite(G_PIN, green);
            analogWrite(B_PIN, blue);
            return;
        }

        float BPMtoMS(int BPM) {
            return 60000/BPM;
        }
};

void printColours(byte red, byte green, byte blue) {
    lcd.clear();
    lcd.print(" R   G   B ");
    
    lcd.setCursor(0,1);
    lcd.print(red);
    
    lcd.setCursor(4,1);
    lcd.print(green);
    
    lcd.setCursor(8,1);
    lcd.print(blue);
    return;
}

void printBPM() {
    // Print a message to the LCD.
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("BPM:");
    lcd.setCursor(0,1);
    lcd.print(BPM);
}

LED rgb_lights;

void setup() {
    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);
    
    pinMode(INC_PIN, INPUT_PULLUP);
    pinMode(DEC_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INC_PIN), incrementInterval, FALLING);
    attachInterrupt(digitalPinToInterrupt(DEC_PIN), decrementInterval, FALLING);

    // Start with a pattern
    rgb_lights.Flash(0.25, 1);

    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    printBPM();
}

void loop() {
    rgb_lights.Update();
}

// Interrupt routines and callbacks

void incrementInterval() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE) {
        ++BPM;
        printBPM();
    }
    lastInterruptTime = interruptTime;
}

void decrementInterval() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE) {
        --BPM;
        printBPM();
    }
    lastInterruptTime = interruptTime;
}