#include <MovingAverage.h>
#include <LiquidCrystal.h>

class MIDI_Sync : private MovingAverage {
    // -------------------- Public variables and functions --------------------
    public:
    
    MIDI_Sync(HardwareSerial *serial_in, byte initial_BPM = 120, unsigned long reset_interval = 3000, void (*callback_beat)() = NULL, void (*callback_reset)() = NULL)
    :MovingAverage(0.2) {

        // Function to call every beat
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
                
                // prev_micros = now_micros;
                // now_micros = micros();
                // interval = now_micros - prev_micros;
                // average.update(60000000.0/interval);

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
    }

    float GetBPM() {
        return MovingAverage::get();
    }

    // -------------------- Private variables and functions --------------------
    private:
    
    // Function to call on every beat
    void (*OnBeat)();

    // Funcation to call on reset
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

// Initialise MIDI_Sync with parameters
MIDI_Sync midi_sync(&Serial1, 128, 3000, &beat, &reset);

// Initialise lcd display with corresponding pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

byte state = 0;
unsigned long prev_time;

void setup() {
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("Setup!");
}

void loop() {
    midi_sync.Update();

    if (millis() - prev_time > 25 && state) {
        analogWrite(31, 0);
        state = 0;
    }
}

void reset() {
    lcd.clear();
    lcd.print("No clock signal!");
}

void beat() {
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print(midi_sync.GetBPM());
    lcd.print(" BPM");
    analogWrite(31, 255);
    state = 1;
    prev_time = millis();
}