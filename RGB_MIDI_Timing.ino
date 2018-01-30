// Define pins for RGB outputs
#define R_PIN 13
#define G_PIN 12
#define B_PIN 11

// Define pin for RGB_Addr outputs and number of LEDs being controlled
#define ADDR_PIN 37
#define N_LEDS 300

// Include required libraries
#include <MovingAverage.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

class MIDI_Sync : private MovingAverage {
    // -------------------- Public variables and functions --------------------
    public:
    
    MIDI_Sync(HardwareSerial *serial_in, byte initial_BPM = 120, unsigned long reset_interval = 3000, void (*callback_tick)() = NULL, void (*callback_beat)() = NULL, void (*callback_reset)() = NULL)
    :MovingAverage(0.2) {

        // Function to call every beat
        OnTick = callback_tick;
        OnBeat = callback_beat;
        OnReset = callback_reset;

        // Set up corresponding serial port with MIDI baud rate
        _Serial = serial_in;
        _Serial->begin(Baud_Rate);

        MovingAverage::reset(initial_BPM);

        Reset_Interval = reset_interval;
    }

    void Update() {
        
        // Check if serial connection is available
        if(_Serial->available() > 0) {

            // Read serial port
            Data = _Serial->read();

            // Check for MIDI data
            if(Data == MIDI_Stop) {
                Counter = 0;
            }
            else if(Data == MIDI_Clock) {
                Sync();
            }
            
            // Set reset flag low
            if (Reset) {
                Reset = 0;
            }
            Last_Call = millis();
        }

        // Check if reset callback specified
        else if( OnReset != NULL) {

            // Check if reset time elapsed
            if (millis() - Last_Call >= Reset_Interval && !Reset) {
                Reset = 1;
                Counter = 0;
                OnReset();
            }
        }
    }

    void Sync() {
        // Check if reached 24th pulse and reset back to 0
        if (Counter == 23) {
            Counter = 0;

            // Check if Now_Millis has been set before for more accurate initial timings
            if (Now_Millis > 0) {
                Prev_Millis = Now_Millis;
                Now_Millis = millis();
                Interval = Now_Millis - Prev_Millis;

                // Update the moving average function
                MovingAverage::update(60000.0/Interval);

                // Call the beat callback if it exists
                if (OnBeat != NULL) {
                    OnBeat();
                }
            }
            else {
                Now_Millis = millis();
            }
        }
        else {
            ++Counter;
        }

        // Call the tick function if it exists (24 per beat);
        if (OnTick != NULL) {
            OnTick();
        }
    }

    float GetBPM() {
        return MovingAverage::get();
    }

    // -------------------- Private variables and functions --------------------
    private:
    
    // Callback functions
    void (*OnTick)();
    void (*OnBeat)();
    void (*OnReset)();

    // Serial object
    HardwareSerial *_Serial = NULL;

    // MIDI signals
    byte MIDI_Start = 0xfa;
    byte MIDI_Stop = 0xfc;
    byte MIDI_Clock = 0xf8;
    byte MIDI_Continue = 0xfb;

    // MIDI baud rate
    int Baud_Rate = 31250;

    // Timing variables
    unsigned long Interval;
    unsigned long Prev_Millis = 0;
    unsigned long Now_Millis = 0;
    unsigned long Last_Call = 0;
    unsigned long Reset_Interval;

    // Other variables
    byte Data;
    byte Counter = 0;;
    bool Reset = 0;
};

class RGB_Colours {
    public:
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

    // Makes a 32 bit colour using three 8 bit RGB values
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

    uint32_t Complement(uint32_t colour) {
        return 0x00FFFFFF - colour;
    }

    uint32_t ShiftRight(uint32_t colour) {
        // Check if colour less than or equal to 0xFF (so blue values)
        // Shift right if true, otherwise wrap around to red
        return (colour <= 0x0000FF) ? colour << 16 : colour >> 8;
    }

    uint32_t ShiftLeft(uint32_t colour) {
        // Check if colour greater than 0xFF00 (so red values)
        // Shift left if true, otherwise wrap around to blue
        return (colour > 0x00FF00) ? colour >> 16 : colour << 8;
    }    
};

class LED_Strip : public RGB_Colours {

    // ---------------------------- PUBLIC ----------------------------

    public:
    
    // Callback when pattern complete
    void (*OnComplete)();

    uint32_t Colour1, Colour2;

    LED_Strip(uint8_t r_pin, uint8_t g_pin, uint8_t b_pin, void (*callback)()) {
        _r_pin = r_pin;
        _g_pin = g_pin;
        _b_pin = b_pin;

        OnComplete = callback;
    }

    void Update() {

        // Update which tick and beat currently on

        // Check if Tick is overflowing
        if (Tick == MaxTicks - 1) {
            Tick = 0;
            
            if (OnComplete != NULL) {
                OnComplete();
            }

            // Check if Beat is overflowing
            if (Beat == MaxBeats - 1) {
                Beat = 0;
            }
            else {
                ++Beat;
            }
        }
        else {
            ++Tick;
        }

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

    // ---------------------------- PATTERN FUNCTIONS ----------------------------

    void Flash(uint32_t initColour1 = 0xFF0000) {
        ActivePattern = FLASH;
        Colour1 = initColour1;
    }

    void Fade(uint32_t initColour1 = 0xFF0000) {
        ActivePattern = FADE;
        Colour1 = initColour1;
    }

    void Strobe(uint32_t initColour1 = 0xFFFFFF) {
        ActivePattern = STROBE;
        Colour1 = initColour1;
    }

    // ---------------------------- PRIVATE ----------------------------

    private:

    // Defining patterns to easily pass them to functions
    enum pattern { FLASH, FADE, STROBE };

    static const uint8_t MaxTicks = 24;
    static const uint16_t MaxBeats = 256;

    pattern ActivePattern;
    uint8_t _r_pin, _g_pin, _b_pin;

    uint8_t Tick = 0;
    uint16_t Beat = 0;

    // ---------------------------- PATTERN UPDATE FUNCTIONS ----------------------------

    void FlashUpdate() {

        // Check if on beat and turn on light
        if (Tick == 0) {
            ColourSet(Colour1);
        }
        // Turn off after short delay
        else if (Tick == 4) {
            ColourSet(0);
        }

    }

    void FadeUpdate() {

        // Calculate linear interpolation between colour1 and colour2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Colour1) * (MaxTicks - 1 - Tick)) + (Red(Colour2) * Tick)) / MaxTicks;
        uint8_t green = ((Green(Colour1) * (MaxTicks - 1 - Tick)) + (Green(Colour2) * Tick)) / MaxTicks;
        uint8_t blue = ((Blue(Colour1) * (MaxTicks - 1 - Tick)) + (Blue(Colour2) * Tick)) / MaxTicks;

        ColourSet(Colour(red, green, blue));
    }

    void StrobeUpdate() {
        
        // Check if on beat and turn on light
        if (Tick == 0) {
            ColourSet(Colour1);
        }
        // Turn off after short delay
        else if (Tick == 2) {
            ColourSet(0);
        }
    }    

    // ---------------------------- UTILITY FUNCTIONS ----------------------------

    void ColourSet(uint32_t colour) {
        analogWrite(_r_pin, Red(colour));
        analogWrite(_g_pin, Green(colour));
        analogWrite(_b_pin, Blue(colour));
    }
};

class LED_Addr : public RGB_Colours {

};

// Initialise MIDI_Sync object
MIDI_Sync midiSync(&Serial1, 128, 3000, &tick, &beat, &reset);

//Initialise LED_Strip object
LED_Strip strip1(R_PIN, G_PIN, B_PIN, &complete);

// Initialise lcd display with corresponding pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

byte state = 0;
unsigned long prev_time;

void setup() {
    strip1.Flash();
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("Setup!");
}

void loop() {
    midiSync.Update();

    if (millis() - prev_time > 100 && state) {
        analogWrite(31, 0);
        state = 0;
    }
}

void tick() {
    strip1.Update();
}

void beat() {
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print(midiSync.GetBPM());
    lcd.print(" BPM");
    analogWrite(31, 255);
    state = 1;
    prev_time = millis();
}

void reset() {
    lcd.clear();
    lcd.print("No clock signal!");
}

void complete() {
    strip1.Colour1 = strip1.ShiftRight(strip1.Colour1);
}