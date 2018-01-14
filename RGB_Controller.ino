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
        // Define patterns and orders
        enum pattern { FLASH, FADE };
        enum order { FORWARD, BACKWARD, RANDOM };
        
        // ---------------------------- VARIABLES ----------------------------

        // Byte array for current RGB values
        byte RGB[3] = { 0, 0, 0 };

        pattern ActivePattern;
        // pattern NextPattern;

        // Variables for timings
        unsigned long onTime;
        unsigned long offTime;
        unsigned long TotalInterval;
        unsigned long Interval;
        unsigned long lastUpdate;

        // Other variables
        byte BPM = 120;
        byte CurrentColNo = 0;
        byte NextColNo = 0;
        byte prevBPM = 0;
        byte BeatDenom = 4;
        order ColourOrder = FORWARD;
        float dutyCycle;
        bool onState;

        // ---------------------------- FUNCTIONS ----------------------------

        void setPattern(pattern inputPattern, order inputOrder, byte inputDenom = 4, float inputDuty = 0.25) {
<<<<<<< HEAD
            // Variables common to all patterns
=======
            // Variables common far all patterns
>>>>>>> c48cc153d41aa72d85483610d44aaa7b88ba7c71
            ActivePattern = inputPattern;
            ColourOrder = inputOrder;
            BeatDenom = inputDenom;
            TotalInterval = calcMS(BeatDenom);

            switch(inputPattern) {
                case FLASH:
                    Flash(inputDuty);
                    break;       
                case FADE:
                    Fade();
                    break;
                default:
                    break;
            }
        }

        byte UpdateBPM(int change) {
            if ((BPM - change >= 0) && (BPM + change <= 255)) {
                BPM += change;
                updateTiming();
            }
            return BPM;
        }

        void updateTiming() {
            TotalInterval = calcMS(BeatDenom);
            onTime = TotalInterval*dutyCycle;
            offTime = TotalInterval - onTime;
        }

        byte getNextColNo() {
            switch(ColourOrder) {
                // Select the next colour number
                case FORWARD:
                    // (R->G->B->R...)
                    return (CurrentColNo < 2) ? CurrentColNo + 1 : 0;
                case BACKWARD:
                    // (B->G->R->B...)
                    return (CurrentColNo > 0) ? CurrentColNo - 1 : 2;
                case RANDOM:
                    // Random colour order;
                    return random(3);
            }
        }

        void Update() {
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
                // Set the lights after updating RGB[]
                setColour(RGB[0], RGB[1], RGB[2]);  
            }
        }

        void Flash(float duty = 0.25) {
            dutyCycle = duty;
            onTime = TotalInterval*dutyCycle;
            offTime = TotalInterval - onTime;
            Interval = onTime;
            CurrentColNo = 0;
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
                CurrentColNo = getNextColNo();
                RGB[CurrentColNo] = 255;
                Interval = onTime;
                onState = true;
            } 
        }

        void Fade() {
            Interval = TotalInterval/17;
            RGB[0] = 255;
            RGB[1] = 0;
            RGB[2] = 0;
            CurrentColNo = 0;
        }

        void FadeUpdate() {
            if (RGB[CurrentColNo] == 255 && RGB[NextColNo] < 255) {
                RGB[NextColNo] += 15;
            }
            else if (RGB[CurrentColNo] > 0 && RGB[NextColNo] == 255) {
                RGB[CurrentColNo] -= 15;
            }
            else if (RGB[CurrentColNo] == 0 && RGB[NextColNo] == 255) {
                CurrentColNo = NextColNo;
                NextColNo = getNextColNo();
            }
            else if (RGB[CurrentColNo] < 255) {
                RGB[CurrentColNo] += 15;
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

// ---------------------------- GLOBAL FUNCTIONS ----------------------------

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

void setup() {
    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);
    
    pinMode(INC_PIN, INPUT_PULLUP);
    pinMode(DEC_PIN, INPUT_PULLUP);
    
    // Initialise BPM for all LED objects

    attachInterrupt(digitalPinToInterrupt(INC_PIN), incrementInterval, FALLING);
    attachInterrupt(digitalPinToInterrupt(DEC_PIN), decrementInterval, FALLING);

    // Start rgb_lights with a pattern
    rgb_lights.setPattern(LED::FLASH, LED::FORWARD, 4, 0.25);

    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    printBPM(rgb_lights.BPM);
}

void loop() {
    rgb_lights.Update();
}

// ---------------------------- INTERRUPT FUNCTIONS ----------------------------

void incrementInterval() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE) {
        printBPM(rgb_lights.UpdateBPM(1));
    }
    lastInterruptTime = interruptTime;
}

void decrementInterval() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE) {
        printBPM(rgb_lights.UpdateBPM(-1));
    }
    lastInterruptTime = interruptTime;
}