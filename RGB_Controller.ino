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
        uint32_t currentColour = 0;
        uint32_t nextColour = 0;
        order ColourOrder = FORWARD;
        byte BeatDenom = 4;
        // TotalSteps for fading, 17 gives step size of 15;
        byte TotalSteps = 17;
        byte currentStep = 0;

        float dutyCycle;
        bool onState;

        // ---------------------------- FUNCTIONS ----------------------------

        void setPattern(pattern inputPattern, order inputOrder, byte inputDenom = 4, float inputDuty = 0.25) {
            // Variables common to all patterns
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

        // ---------------------------- BPM FUNCTIONS ----------------------------

        byte UpdateBPM(int change) {
            if ((BPM - change >= 0) && (BPM + change <= 255)) {
                BPM += change;
                updateTiming();
            }
            return BPM;
        }

        void updateTiming() {
            TotalInterval = calcMS(BeatDenom);
            if (ActivePattern == FLASH) {
                onTime = TotalInterval*dutyCycle;
                offTime = TotalInterval - onTime;
            }
        }

        float calcMS(byte denom) {
            return 60000/(BPM*denom);
        }

        // ---------------------------- COLOUR FUNCTIONS ----------------------------

        // Returns the Red component of a 32-bit color
        uint8_t Red(uint32_t color)
        {
            return (color >> 16) & 0xFF;
        }

        // Returns the Green component of a 32-bit color
        uint8_t Green(uint32_t color)
        {
            return (color >> 8) & 0xFF;
        }

        // Returns the Blue component of a 32-bit color
        uint8_t Blue(uint32_t color)
        {
            return color & 0xFF;
        }

        uint32_t getNextColour() {
            switch(ColourOrder) {
                // Select the next colour number
                case FORWARD:
                    // (R->G->B->R...)
                    if (currentColour > 0x0000FF) {
                        return currentColour >> 8;
                    }
                    else {
                        return 0xFF0000;
                    }
                case BACKWARD:
                    // (B->G->R->B...)
                    if (currentColour < 0xFF0000) {
                        return currentColour << 8;
                    }
                    else {
                        return 0x0000FF;
                    }
                case RANDOM:
                    // Random colour;
                    return (random(255) << 16 | random(255) << 8 | random(255));
            }
        }

        uint32_t Colour(uint8_t r, uint8_t g, uint8_t b) {
            return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
        }

        void setColour(uint32_t inputColour) {
            analogWrite(R_PIN, Red(inputColour));
            analogWrite(G_PIN, Green(inputColour));
            analogWrite(B_PIN, Blue(inputColour));
            return;
        }

        // ---------------------------- PATTERN FUNCTIONS ----------------------------

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
            }
        }

        void Flash(float duty = 0.25) {
            dutyCycle = duty;
            onTime = TotalInterval*dutyCycle;
            offTime = TotalInterval - onTime;
            Interval = onTime;
            currentColour = 0xFF0000;
            onState = false;
        }

        void FlashUpdate() {
            // Check if lights are in ON or OFF state
            if(onState) {
                // Turn the lights off
                setColour(0);
                currentColour = getNextColour();
                Interval = offTime;
                onState = false;
            }
            else {
                setColour(currentColour);
                Interval = onTime;
                onState = true;
            } 
        }

        void Fade() {
            Interval = TotalInterval/TotalSteps;
            currentColour = 0xFF0000;
            nextColour = getNextColour();
            currentStep = 0;
        }

        void FadeUpdate() {
            // Calculate linear interpolation between Color1 and Color2
            // Optimise order of operations to minimize truncation error
            if (currentStep >= TotalSteps) {
                nextColour = getNextColour();
                currentStep = 0;
            }
            uint8_t red = ((Red(currentColour) * (TotalSteps - currentStep)) + (Red(nextColour) * currentStep)) / TotalSteps;
            uint8_t green = ((Green(currentColour) * (TotalSteps - currentStep)) + (Green(nextColour) * currentStep)) / TotalSteps;
            uint8_t blue = ((Blue(currentColour) * (TotalSteps - currentStep)) + (Blue(nextColour) * currentStep)) / TotalSteps;
            
            setColour(Colour(red, green, blue));
            ++currentStep;
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