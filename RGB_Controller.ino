// Define pins for BPM button inputs
#define INC_PIN 20
#define DEC_PIN 21

// Define pins for RGB outputs
#define R_PIN 13
#define G_PIN 12
#define B_PIN 11

// Define pin for RGB_Addr outputs and number of LEDs
#define ADDR_PIN 31
#define N_LEDS 300

// Debounce time for inputs
#define DEBOUNCE 100

// Including library code
#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

class LED {
    public:

    // Define order
    enum order { RGB, BGR, WHEEL, RANDOM };

    order ColourOrder = RGB;

    // enum bpm_source { MANUAL, MIDI };

    // ---------------------------- VARIABLES ----------------------------
    // pattern NextPattern;

    // Variables for timings
    unsigned long onTime;
    unsigned long offTime;
    unsigned long TotalInterval;
    unsigned long Interval;
    unsigned long lastUpdate;

    // Other variables
    byte BPM = 120;
    byte BeatDenom = 4;
    // TotalSteps for fading, 17 gives step size of 15;
    byte TotalSteps = 17;
    byte currentStep = 0;

    uint32_t currentColour = 0;
    uint32_t nextColour = 0;

    byte red;
    byte green;
    byte blue;

    float dutyCycle;
    bool onState;

    // ---------------------------- PATTERN CONTROL FUNCTIONS ----------------------------

    // Function to set a pattern with other parameters (some parameters only needed for certain functions)
    // virtual void setPattern(pattern inputPattern, order inputOrder = RGB, byte inputDenom = 4, float inputDuty = 0.25) = 0;
    // virtual void Update() = 0;

    // ---------------------------- BPM FUNCTIONS ----------------------------

    byte UpdateBPM(int change) {
        if ((BPM + change >= 0) && (BPM + change <= 255)) {
            BPM += change;
            updateTiming();
        }
        return BPM;
    }

    virtual void updateTiming() = 0;

    float calcMS(byte denom) {
        return 4*60000/(BPM*denom);
    }

    // ---------------------------- COLOUR FUNCTIONS ----------------------------

    // Returns the Red component of a 32-bit colour
    uint8_t Red(uint32_t colour)
    {
        return (colour >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit colour
    uint8_t Green(uint32_t colour)
    {
        return (colour >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit colour
    uint8_t Blue(uint32_t colour)
    {
        return colour & 0xFF;
    }

    uint32_t getNextColour() {
        switch(ColourOrder) {
            // Select the next colour number
            case RGB:
                // (R->G->B->R...)
                if (currentColour > 0x0000FF) {
                    return uint32_t(currentColour >> 8);
                }
                else {
                    return uint32_t(0xFF0000);
                }
            case BGR:
                // (B->G->R->B...)
                if (currentColour < 0xFF0000) {
                    return uint32_t(currentColour << 8);
                }
                else {
                    return uint32_t(0x0000FF);
                }
            case WHEEL:
                // Use a colour wheel to determine the next colour
                return Wheel(random(255));
            case RANDOM:
                // Random colour;
                return Colour(random(255), random(255), random(255));
        }
    }

    uint32_t Colour(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
    }

    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Colour(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Colour(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Colour(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

// ---------------------------- RGB CLASS ----------------------------

class RGB : public LED {
    public:

    // Define patterns for non-addressable LEDs
    enum pattern { FLASH, FADE, STROBE };
    
    pattern ActivePattern;

    // ---------------------------- PATTERN CONTROL FUNCTIONS ----------------------------

    // Function to set a pattern with other parameters (some parameters only needed for certain functions)
    void setPattern(pattern inputPattern, order inputOrder = LED::RGB, byte inputDenom = 4, float inputDuty = 0.25) {
        
        // Set up variables common to all patterns
        ActivePattern = inputPattern;
        ColourOrder = inputOrder;
        BeatDenom = inputDenom;
        TotalInterval = calcMS(BeatDenom);

        // Set up specific variables for each different pattern
        switch(inputPattern) {
            case FLASH:
                Flash(inputDuty);
                break;
            case FADE:
                Fade();
                break;
            case STROBE:
                Strobe(inputDuty);
                break;
            default:
                break;
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
                case STROBE:
                    StrobeUpdate();
                    break;
                default:
                    break;
            }
        }
    }

    void updateTiming() {
        TotalInterval = calcMS(BeatDenom);
        switch (ActivePattern) {
            case FLASH:
                onTime = TotalInterval*dutyCycle;
                offTime = TotalInterval - onTime;
                break;
            case FADE:
                Interval = TotalInterval/TotalSteps;
                break;
        }
    }

    // ---------------------------- PATTERN FUNCTIONS ----------------------------

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
        if (currentStep >= TotalSteps) {
            currentColour = nextColour;
            nextColour = getNextColour();
            currentStep = 0;
        }

        // Calculate linear interpolation between colour1 and colour2
        // Optimise order of operations to minimize truncation error
        red = ((Red(currentColour) * (TotalSteps - currentStep)) + (Red(nextColour) * currentStep)) / TotalSteps;
        green = ((Green(currentColour) * (TotalSteps - currentStep)) + (Green(nextColour) * currentStep)) / TotalSteps;
        blue = ((Blue(currentColour) * (TotalSteps - currentStep)) + (Blue(nextColour) * currentStep)) / TotalSteps;

        setColour(Colour(red, green, blue));
        ++currentStep;
    }

    void Strobe(float duty = 0.1) {
        dutyCycle = duty;
        onTime = TotalInterval*dutyCycle;
        offTime = TotalInterval - onTime;
        Interval = onTime;
        currentColour = 0xFFFFFF;
        onState = false;
    }

    void StrobeUpdate() {
        // Check if lights are in ON or OFF state
        if(onState) {
            // Turn the lights off
            setColour(0);
            Interval = offTime;
            onState = false;
        }
        else {
            setColour(currentColour);
            Interval = onTime;
            onState = true;
        } 
    }

    // ---------------------------- LED CONTROL FUNCTIONS ----------------------------

    void setColour(uint32_t inputColour) {
        analogWrite(R_PIN, Red(inputColour));
        analogWrite(G_PIN, Green(inputColour));
        analogWrite(B_PIN, Blue(inputColour));
        return;
    }
};

// ---------------------------- ADDRESSABLE RGB CLASS ----------------------------

class RGB_Addr : public LED, public Adafruit_NeoPixel {
    public:

    // ---------------------------- VARIABLES ----------------------------

    // Pattern types supported:
    enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
    // Directions supported:
    enum  direction { FORWARD, REVERSE };

    pattern ActivePattern;
    
    direction Direction;     // direction to run the pattern    

    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    RGB_Addr(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }

    // Function to set a pattern with other parameters (some parameters only needed for certain functions)
    // void setPattern(pattern inputPattern, direction dir = FORWARD, byte inputDenom = 4, float inputDuty = 0.25) {
        
    //     // Set up variables common to all patterns
    //     ActivePattern = inputPattern;
    //     Direction = dir;
    //     BeatDenom = inputDenom;
    //     TotalInterval = calcMS(BeatDenom);

    //     // Set up specific variables for each different pattern
    //     switch(inputPattern) {
    //         case RAINBOW_CYCLE:
    //             RainbowCycle();
    //             break;
    //         case THEATER_CHASE:
    //             TheaterChase();
    //             break;
    //         case COLOR_WIPE:
    //             ColorWipe();
    //             break;
    //         case SCANNER:
    //             Scanner();
    //             break;
    //         case FADE:
    //             Fade();
    //             break;
    //         case NONE:
    //             None();
    //             break;
    //         default:
    //             break;
    //     }
    // }

    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                case NONE:
                    break;
                default:
                    break;
            }
        }
    }

    void updateTiming() {
        TotalInterval = calcMS(BeatDenom);
        // switch (ActivePattern) {
        //     case FLASH:
        //         onTime = TotalInterval*dutyCycle;
        //         offTime = TotalInterval - onTime;
        //         break;
        //     case FADE:
        //         Interval = TotalInterval/TotalSteps;
        //         break;
        // }
    }

    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }

    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, currentColour);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        show();
        Increment();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        // TotalSteps = (numPixels() - 1) * 2;
        TotalSteps = (numPixels()-1)/4;
        Color1 = color1;
        Index = 0;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++) {
            if (i % TotalSteps == Index) {
                setPixelColor(i, Color1);
            }
            else if (i % TotalSteps == TotalSteps - Index) {
                setPixelColor(i, Color1);
            }
            else {
                setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }
   
    void None() {
        ColorSet(0);
        show();
    }

    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
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

RGB rgb_lights;
RGB_Addr Stick(N_LEDS, ADDR_PIN, NEO_GRB + NEO_KHZ800, &StickComplete);

// ---------------------------- MAIN SETUP ----------------------------

void setup() {
    Serial.begin(9600);
    Serial.println("Sending serial data");
    
    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);
    
    pinMode(INC_PIN, INPUT_PULLUP);
    pinMode(DEC_PIN, INPUT_PULLUP);
    
    // Initialise BPM for all LED objects

    attachInterrupt(digitalPinToInterrupt(INC_PIN), incrementInterval, FALLING);
    attachInterrupt(digitalPinToInterrupt(DEC_PIN), decrementInterval, FALLING);

    // Start rgb_lights with a pattern
    // rgb_lights.setPattern(LED::FADE, LED::FORWARD, 8, 0.25);
    rgb_lights.setPattern(RGB::FLASH, LED::WHEEL, 8, 0.25);
    // rgb_lights.setPattern(LED::STROBE, LED::FORWARD, 8);
    
    Stick.begin();
    Stick.Scanner(Stick.Color(237, 22, 140), 1);

    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    printBPM(rgb_lights.BPM);
}

// ---------------------------- MAIN LOOP ----------------------------

void loop() {
    rgb_lights.Update();
    Stick.Update();
    // Send some data to the serial monitor for debugging
    // Serial.println(rgb_lights.red);
    // Serial.println(rgb_lights.green);
    // Serial.println(rgb_lights.blue);
    // Serial.println(rgb_lights.currentStep);
    // Serial.println(rgb_lights.currentColour);
    // Serial.println(rgb_lights.nextColour);
}

// ---------------------------- CALLBACK FUNCTIONS ----------------------------

// Stick Completion Callback
void StickComplete()
{
    // Random color change for next scan
    Stick.Color1 = Stick.Wheel(random(255));
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