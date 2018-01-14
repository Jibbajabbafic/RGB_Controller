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

class LED {
    public:
        // BPM for all instances of LED
        static byte BPM;

        // Byte array for current RGB values
        byte RGB[3] = {0, 0, 0};
        // Types of pattern
        enum pattern { FLASH, FADE };
        
        // VARIABLES
        pattern ActivePattern;
        // pattern NextPattern;

        // Variables for timings
        unsigned long onTime;
        unsigned long offTime;
        unsigned long TotalInterval;
        unsigned long Interval;
        unsigned long lastUpdate;

        // Other variables
        byte colourNo = 0;
        byte prevBPM = 0;
        byte BeatDenom = 4;
        float dutyCycle;
        bool onState;

        static int UpdateBPM(int change) {
            BPM += change;
            return BPM;
        }

        void updateTiming() {
            TotalInterval = calcMS(BeatDenom);
            onTime = TotalInterval*dutyCycle;
            offTime = TotalInterval - onTime;
        }

        byte getNextColour() {
            // Select the next colour number (R->G->B->R...)
            return colourNo < 2 ? colourNo + 1 : 0;
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
                    FadeUpdate();
                    break;
                default:
                    break;
                }
                // Set the lights
                setColour(RGB[0], RGB[1], RGB[2]);  
            }
        }

        void Flash(float duty = 0.25, byte beatDenom = 4) {
            ActivePattern = FLASH;
            BeatDenom = beatDenom;
            TotalInterval = calcMS(BeatDenom);
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
                colourNo = getNextColour();
                RGB[colourNo] = 255;
                Interval = onTime;
                onState = true;
            } 
        }

        void Fade(byte beatDenom = 1) {
            ActivePattern = FADE;
            BeatDenom = beatDenom;
            TotalInterval = calcMS(BeatDenom);
            Interval = TotalInterval/17;
            RGB[0] = 255;
            RGB[1] = 0;
            RGB[2] = 0;
            colourNo = 0;
        }

        void FadeUpdate() {
            if (RGB[colourNo] == 255 && RGB[getNextColour()] < 255) {
                RGB[getNextColour()] += 15;
            }
            else if (RGB[colourNo] > 0 && RGB[getNextColour()] == 255) {
                RGB[colourNo] -= 15;
            }
            else if (RGB[colourNo] == 0 && RGB[getNextColour()] == 255) {
                colourNo == getNextColour();
            }
        }

        void setColour(byte red, byte green, byte blue) {
            analogWrite(R_PIN, red);
            analogWrite(G_PIN, green);
            analogWrite(B_PIN, blue);
            return;
        }

        float calcMS(byte denom) {
            return 60000/(BPM*denom);
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

void printBPM(int bpm) {
    // Print a message to the LCD.
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("BPM:");
    lcd.setCursor(0,1);
    lcd.print(bpm);
}

LED rgb_lights;

byte LED::BPM = 120;

void setup() {
    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);
    
    pinMode(INC_PIN, INPUT_PULLUP);
    pinMode(DEC_PIN, INPUT_PULLUP);
    
    // Initialise BPM for all LED objects

    attachInterrupt(digitalPinToInterrupt(INC_PIN), incrementInterval, FALLING);
    attachInterrupt(digitalPinToInterrupt(DEC_PIN), decrementInterval, FALLING);

    // Start with a pattern
    rgb_lights.Flash(0.25, 1);

    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    printBPM(LED::BPM);
}

void loop() {
    rgb_lights.Update();
}

// Interrupt routines and callbacks

void incrementInterval() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE) {
        printBPM(LED::UpdateBPM(1));
    }
    lastInterruptTime = interruptTime;
}

void decrementInterval() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE) {
        printBPM(LED::UpdateBPM(-1));
    }
    lastInterruptTime = interruptTime;
}