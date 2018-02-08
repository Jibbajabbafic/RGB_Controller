#include <Adafruit_NeoPixel.h>
// #ifdef __AVR__
//   #include <avr/power.h>
// #endif

// define pins for X: RGB outputs
#define X_R_PIN 9
#define X_G_PIN 10
#define X_B_PIN 1

// define pins for Addressable: RGB outputs
#define ADDR_PIN 5
#define N_LEDS 300


class LED {
    public:

    // define order
    enum order { RGB, BGR, WHEEL, RANDOM };
    order colorOrder = RGB;

    enum bpmSource { MANUAL, MIDI };

    unsigned long onTime, offTime, totalInterval, interval, lastUpdate, duration;
    byte BPM = 120, beatDenom = 4, totalSteps = 17, currentStep = 0;
    uint32_t currentColor = 0, nextColor = 0;
    byte red, green, blue;
    float dutyCycle;
    bool onState;

    // BPM Functions 

    byte updateBPM(int change) {
      if ((BPM + change >= 0) && (BPM + change <= 255)){
         BPM += change;
         //updateTiming();
      }
      return BPM;
    }

    //virtual void updateTiming()=0;

    float calcMS(byte denom) {
        return (4*60000/(BPM*denom))/2;
    }

    // ----------------- color functions -----------------------
    
    // return red component of 32-bit color
    uint8_t redComp(uint32_t col){
        return (col >> 16) & 0xFF;
    }
    // return green component of 32-bit color
    uint8_t greenComp(uint32_t col){
        return (col >> 8) & 0xFF;
    }
    // return blue component of 32-bit color
    uint8_t blueComp(uint32_t col){
        return col & 0xFF;
    }
    // return 32 bit color from rgb
    uint32_t fullColor(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }

    uint32_t introp(uint32_t col1, uint32_t col2, uint16_t idx, uint16_t ttS) { 
        uint8_t r = ((redComp(col1)*(ttS-idx))+(redComp(col2)*idx))/ttS;
        uint8_t g = ((greenComp(col1)*(ttS-idx))+(greenComp(col2)*idx))/ttS;
        uint8_t b = ((blueComp(col1)*(ttS-idx))+(blueComp(col2)*idx))/ttS;
        return fullColor(r,g,b);
    }

    uint32_t wheel(byte wheelPos) {
        wheelPos = 255 - wheelPos;
        if (wheelPos < 85){
            return fullColor(255-wheelPos*3, 0, wheelPos*3);
        } else if (wheelPos < 170){
            wheelPos -= 85;
            return fullColor(0, wheelPos*3, 255-wheelPos*3);
        } else {
            wheelPos -= 170;
            return fullColor(wheelPos*3, 255-wheelPos*3, 0);
        }
    }

    uint32_t getNextColor(){
        switch(colorOrder){
            // select next color number
            case RGB:
              // if (R->G->B->R...)
              if (currentColor > 0x0000FF) {
                  return uint32_t(currentColor >> 8);
              } else {
                  return uint32_t(0xFF0000);
              }
            case BGR:
               if (currentColor < 0xFF0000) {
                  return uint32_t(currentColor << 8);
               } else {
                  return uint32_t(0x0000FF);
               }
            case WHEEL:
               return wheel(random(255));
            case RANDOM:
               return fullColor(random(255), random(255), random(255));
        }
    }
};

// ----------------- [NON ADDRESSABLE] RGB CLASS ---------------------
class XRGB : public LED {
    public:

     // patterns for non-addressable LEDS
     enum xRGBPattern { FLASH, FADE, STROBE, NONE, DISPLAYCOL};
     xRGBPattern xRGBActivePattern;

     void (*OnComplete)(); // callback on compleition 
     
     // ----------------- PATTERN CONTROL FUNCTIONS --------------------------
     void xRGBDisplayColor(){
        setColor(currentColor);   
     }
     void xRGBDisplayColorUpdate(){
      
     }
     
     void xRGBFlash (float duty){
        dutyCycle = duty;
        onTime = totalInterval*dutyCycle;
        offTime = totalInterval - onTime;
        interval = onTime;
        currentColor = 0xFF0000;
        onState = false;
     }

     void xRGBFlashUpdate(){
        if (onState) {
            setColor(0);
            currentColor = getNextColor();
        } else {
            setColor(currentColor);
            interval = onTime;
            onState = true;
        }
     }

     void xRGBFade() { 
        interval = totalInterval/totalSteps;
        currentColor = 0xFF0000;
        nextColor = getNextColor();
        currentStep = 0;
     }

     void xRGBFadeUpdate() { 
        if (currentStep >= totalSteps) {
            currentColor = nextColor;
            nextColor = getNextColor();
            currentStep = 0;
        }


        // linear interpolation between color 1 & color 2;
        
        uint32_t introped = introp(currentColor,nextColor,totalSteps,currentStep);
        setColor(introped);
        ++currentStep;
     }

     void xRGBStrobe(float duty) {
        // strobe in the pink of the axone vibez
        dutyCycle = duty;
        onTime = totalInterval*dutyCycle;
        offTime = totalInterval-onTime;
        interval = onTime;
        currentColor = fullColor(229,0,123);
        onState = false;
     }

     void xRGBStrobeUpdate(){
        if(onState) {
            setColor(0);
            interval = offTime;
            onState = false;
        } else {
            setColor(currentColor);
            interval = onTime;
            onState = true;
        }
     }

     void xRGBBlank(){
         setColor(0);
         
     }
     void xRGBBlankUpdate(){
         
     }
     
     void setXRGBPattern(xRGBPattern inputPattern, order inputOrder = LED::RGB, byte inputDenom = 4, float inputDuty = 0.25, uint32_t dur = 5000000) {
        xRGBActivePattern = inputPattern;
        colorOrder = inputOrder;
        beatDenom = inputDenom;
        totalInterval = calcMS(beatDenom);
        duration = dur;
        switch (inputPattern) {
            case FLASH: 
                xRGBFlash(inputDuty);
                break;
            case FADE:
                xRGBFade();
                break;
            case STROBE:
                xRGBStrobe(inputDuty);
                break;
            case NONE:
                xRGBBlank();
                break;
            case DISPLAYCOL:
                xRGBDisplayColor();
                break;
            default:
                break;
        }
     }

     void xRGBUpdate(){
        
        if ((millis()-lastUpdate)>interval){
           lastUpdate = millis();
           // if the last update 
           switch(xRGBActivePattern){
                case FLASH:
                    xRGBFlashUpdate();
                    break;
                case FADE:
                    xRGBFadeUpdate();
                    break;
                case STROBE:
                    xRGBStrobeUpdate();
                    break;
                case NONE:
                    xRGBBlankUpdate();
                    break;
                case DISPLAYCOL:
                    xRGBDisplayColorUpdate();
                    break;
                default:
                    break;
           }
        }
     }

     // ----- LED CONTROL FUNCTIONS ------
     // set the color 
     void setColor(uint32_t inputColor){
        analogWrite(X_R_PIN, redComp(inputColor));
        analogWrite(X_G_PIN, greenComp(inputColor));
        analogWrite(X_B_PIN, blueComp(inputColor));
        return;
     }
};

// ------------------------ ADDRESSABLE RGB CLASS -----------------------------

class RGB_ADDR : public LED, public Adafruit_NeoPixel {
    public:

     // --- variables
     enum addrDirection { FORWARD, REVERSE };
     enum addrPattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, MULTISCANNER, FADE };
     addrPattern addrActivePattern; // active pattern
     addrDirection addrActiveDirection; // active direction
     uint32_t addrCol1, addrCol2;
     uint16_t addrTotalSteps, addrIdx;
     void (*OnComplete)(); // callback on completion of pattern;
     // constructor - calls base-class constrouctor to intialise strip
     RGB_ADDR(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)()):Adafruit_NeoPixel(pixels, pin, type){
         OnComplete = callback;
     }
   
     void addrUpdate() {
         if ((millis()-lastUpdate)>interval){
             lastUpdate = millis();
             switch(addrActivePattern){
                 
             }
         }
     }
 
     void increment() {
         if (addrActiveDirection == FORWARD) {
             addrIdx++;
             if (addrIdx >= totalSteps) {
                 addrIdx = 0;
                 if (OnComplete != NULL) {
                     OnComplete();
                 }
             }
         } else { // reverse
            --addrIdx;
            if (addrIdx <= 0){
               addrIdx = totalSteps-1;
               if (OnComplete != NULL) {
                   OnComplete();
               }
            }
         }
     }
 
     void reversePattern() {
         if (addrActiveDirection = FORWARD){
            addrIdx = totalSteps-1;
         } else {
            addrIdx = 0;
         }
     }
     // initaliase rainbow cycle
     void rainbowCycle(uint8_t ivl, addrDirection dir = FORWARD) {
         addrActivePattern = RAINBOW_CYCLE;
         interval = ivl;
         addrTotalSteps = 255;
         addrIdx = 0;
         addrActiveDirection = dir;
     }
     // update rainbow cycle
     void rainbowCycleUpdate() {
         for (int i = 0; i < numPixels(); i++) {
             setPixelColor(i, wheel(((i*256/numPixels()) + addrIdx) & 255));
         }
         show();
         increment();
     }
     // initialise theater chase
     void theaterChase(uint32_t col1, uint32_t col2, uint8_t ivl, addrDirection dir = FORWARD) {
         addrActivePattern = THEATER_CHASE;
         interval = ivl;
         addrTotalSteps = numPixels();
         addrCol1 = col1;
         addrCol2 = col2;
         addrIdx = 0;
         addrActiveDirection = dir;
     }
     // update theater chase 
     void theaterChaseUpdate() {
         for (int i=0; i<numPixels(); i++) {
             if (( i + addrIdx) % 3 == 0) {
                 setPixelColor(i, currentColor);
             } else {
                 setPixelColor(i, addrCol2);
             }
         }
         show();
         increment();
     }
     // initialise colorwipe
     void colorWipe(uint32_t col, uint8_t ivl, addrDirection dir = FORWARD) {
         addrActivePattern = COLOR_WIPE;
         interval = ivl;
         addrTotalSteps = numPixels();
         addrCol1 = col;
         addrIdx = 0;
         addrActiveDirection = dir;
     }
     // update color wipe
     void colorWipeUpdate() {
         setPixelColor(addrIdx, addrCol1);
         show();
         increment();
     }
     // initialise Scanner
     void scanner(uint32_t col1, uint8_t ivl) {
         addrActivePattern = SCANNER;
         interval = ivl;
         addrTotalSteps = (numPixels() - 1) * 2;
         addrCol1 = col1;
         addrIdx = 0;
     }
     // update scanner
     void scannerUpdate() { 
         for (int i = 0; i < numPixels(); i++) {
             if (i = addrIdx) { // scan pixel on right
                 setPixelColor(i, addrCol1);
             } else if (i == addrTotalSteps - addrIdx) { // scan pixel to left
                 setPixelColor(i, addrCol1);
             } else { // fading tail
                 setPixelColor(i, dimColor(getPixelColor(i)));
             }
         }
         show();
         increment();
     }
     // intialise multiscanner
     void multiScanner(uint32_t col1, uint32_t col2, uint8_t sections, uint8_t ivl) { 
         addrActivePattern = MULTISCANNER;
         interval = ivl;
         addrTotalSteps = numPixels()/sections;
         addrCol1 = col1;
         addrCol2 = col2;
         addrIdx = 0;
     }
     // update multiscanner
     void multiScannerUpdate() {
         for (int i = 0; i < numPixels(); i++) { 
             if (i%addrTotalSteps == addrIdx){
                 setPixelColor(i, addrCol1);
             } else if (i%addrTotalSteps == (addrTotalSteps - addrIdx)) {
                 setPixelColor(i, addrCol2);
             } else {
                 setPixelColor(i, dimColor(getPixelColor(i)));
             }
         }
         show();
         increment();
         if (addrIdx == addrTotalSteps/2){
             if (OnComplete != NULL) {
                 OnComplete();
             }
         }
     }
     // initialise a fade 
     void fade(uint32_t col1, uint32_t col2, uint16_t steps, uint8_t ivl, addrDirection dir = FORWARD) {
         addrActivePattern = FADE;
         interval = ivl;
         addrTotalSteps = steps;
         addrCol1 = col1;
         addrCol2 = col2;
         addrIdx = 0;
         addrActiveDirection = dir;
     }
     // fade update
     void fadeUpdate() { 
         // linear intropolation
         uint32_t introped = introp(addrCol1, addrCol2, addrTotalSteps, addrIdx);
         colorSet(introped);
         show();
         increment();
     }
 
     void noPattern() { 
         colorSet(0);
         show();
     }
 
     void colorSet(uint32_t col) {
         for (int i = 0; i < numPixels(); i++) { 
             setPixelColor(i, col);
         }
         show();
     }
 
     uint32_t dimColor(uint32_t col) {
         uint32_t dimCol = Color(redComp(col) >> 1, greenComp(col) >> 1, blueComp(col) >> 1);
         return dimCol;
     }
 
     void updateAdr() {
         float z = millis() - lastUpdate;
         duration -= z;
 
         // dont update if duration has expired
         if (duration <= 0 ){
            return;
         }
         
         if (z > interval) {
             lastUpdate = millis();
             switch(addrActivePattern) {
                 case RAINBOW_CYCLE:
                     rainbowCycleUpdate();
                     break;
                 case THEATER_CHASE:
                     theaterChaseUpdate();
                     break;
                 case COLOR_WIPE:
                     colorWipeUpdate();
                     break;
                 case SCANNER:
                     scannerUpdate();
                     break;
                 case MULTISCANNER:
                     multiScannerUpdate();
                     break;
                 case FADE:
                     fadeUpdate();
                     break;
                 case NONE:
                     break;
                 default:
                     break;
             }
         }
     }
};



// -------------------------------------------------------------------------------------------
// ------------------------------- MAIN SETUP ------------------------------------------------
// -------------------------------------------------------------------------------------------


void aCompletionCall();
void randomColorChangeCompletion();
void completeScannerGoMultiScanner();
void completeMultiScannerGoToFade();


RGB_ADDR addressStrip(N_LEDS, ADDR_PIN, NEO_GRB + NEO_KHZ800, &aCompletionCall);
XRGB xStrip;



// INITIAL SETUP
void setup() {

    pinMode(X_R_PIN, OUTPUT);
    pinMode(X_G_PIN, OUTPUT);
    pinMode(X_B_PIN, OUTPUT);

    addressStrip.begin();
    
}

void loop() {
  // put your main code here, to run repeatedly:

    xStrip.xRGBUpdate();
    addressStrip.updateAdr();
    
    unsigned long currentSecond = millis()/1000;
    
 
   
    if (currentSecond <= 1800){
        // if under 30 mins lets do nothing
    } else if (currentSecond > 1800 && currentSecond <= 2100) {
        // if over 30 mins; we add a slow scanner, with a callback which will switch on the x for a set duration, this will go along for 5 mins maybe
        if(addressStrip.addrActivePattern != RGB_ADDR::SCANNER) {
            addressStrip.scanner(addressStrip.fullColor(255,255,255), 100);
            addressStrip.OnComplete = &firstStrobeCallback;
        }
    } else if (currentSecond > 2100 && currentSecond <= 2700) {
        // 35 mins in- make everything clear again
        if(addressStrip.addrActivePattern != RGB_ADDR::NONE) { 
            addressStrip.noPattern();
            addressStrip.OnComplete = NULL;
            xStrip.setXRGBPattern(XRGB::NONE, LED::RANDOM);
        }
    } else if (currentSecond > 2700 && currentSecond <= 3600) {
        // 45 mins in- slightly faster scanner, in white
        if (addressStrip.addrActivePattern != RGB_ADDR::SCANNER) {
            addressStrip.scanner(addressStrip.fullColor(255,255,255), 50);
            addressStrip.OnComplete = NULL;
        }
    } else if (currentSecond > 3600 && currentSecond <= 4200) {
        // an hour in (to 1h10 in)
        if (addressStrip.OnComplete == NULL){
            addressStrip.OnComplete = &completeScannerGoMultiScanner;
        }

        // switch the X on at 1:03 in
        // show in 30 second intervals
        xStrip.currentColor = xStrip.fullColor(229,0,123);
        
        if (currentSecond > 3780 && currentSecond <= 3810) {
            xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if (currentSecond > 3810 && currentSecond <= 3840) {
            xStrip.setXRGBPattern(XRGB::NONE, LED::RGB);
        } else if (currentSecond > 3840 && currentSecond <= 3870) {
            xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if (currentSecond > 3870 && currentSecond <= 3900) {
            xStrip.setXRGBPattern(XRGB::NONE, LED::RGB);
        } else if (currentSecond > 3900 && currentSecond <= 4200) {
            xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } 
    } else if (currentSecond > 4200 && currentSecond <= 4260) {
        // color fade addressable square to that same pink as the X and maintain this for a min
        if (addressStrip.OnComplete != completeMultiScannerGoToColorWipe){
            addressStrip.OnComplete = &completeMultiScannerGoToColorWipe;
        }
        // erm this is 70 mins in
    } else if (currentSecond > 4260 && currentSecond <= 4800) {
        //71-75 min in, clear everything
        xStrip.setXRGBPattern(XRGB::NONE, LED::RGB);
        if (addressStrip.addrActivePattern != RGB_ADDR::NONE) {
            addressStrip.noPattern();
            addressStrip.OnComplete = NULL;
        }
    } else if (currentSecond > 4800 && currentSecond <= 5100) {
        // 75-80 mins in, do a little blue scanner
        if (addressStrip.addrActivePattern != RGB_ADDR::SCANNER){
            addressStrip.scanner(addressStrip.fullColor(38,181,253), 60);
        }
    } else if (currentSecond > 5100 && currentSecond <= 5400) {
        // 85-90 mins in
        if (addressStrip.OnComplete == NULL) {
            addressStrip.OnComplete = &completeScannerGoMultiScanner;
        }
        if (addressStrip.OnComplete == completeScannerGoMultiScanner && currentSecond > 5103 && currentSecond < 5105){
            addressStrip.OnComplete = &randomColorChangeCompletion;
        }
    } else if (currentSecond > 5400 && currentSecond <= 6000) {
        // 90 - 100 mins in; jus play w the X
        if (addressStrip.addrActivePattern != RGB_ADDR::NONE) {
            addressStrip.noPattern();
            addressStrip.OnComplete = NULL;
        }

        if (currentSecond > 5520 && currentSecond <= 5580) {
           xStrip.currentColor = xStrip.fullColor(229,0,123);
           xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if (currentSecond > 5580 && currentSecond <= 5610) {
           xStrip.currentColor = xStrip.fullColor(255,255,255);
           xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if (currentSecond > 5610 && currentSecond <= 5640) {
           xStrip.currentColor = xStrip.fullColor(0,0,0);
           xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if ((currentSecond > 5640 && currentSecond <= 5645) || (currentSecond > 5650 && currentSecond <= 5655) || (currentSecond > 5660 && currentSecond <= 5665) || (currentSecond > 5670 && currentSecond <= 5675) || (currentSecond > 5680 && currentSecond <= 5685) || (currentSecond > 5690 && currentSecond <= 5695) || (currentSecond > 5700 && currentSecond <= 5705) || (currentSecond > 5710 && currentSecond <= 5715) || (currentSecond > 5720 && currentSecond <= 5725) || (currentSecond > 5730 && currentSecond <= 5735) || (currentSecond > 5740 && currentSecond <= 5745) || (currentSecond > 5750 && currentSecond <= 5770)){
           xStrip.currentColor = xStrip.fullColor(229,0,123);
           xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if ((currentSecond > 5645 && currentSecond <= 5650) || (currentSecond > 5655 && currentSecond <= 5660) || (currentSecond > 5665 && currentSecond <= 5670) || (currentSecond > 5675 && currentSecond <= 5680) || (currentSecond > 5685 && currentSecond <= 5690) || (currentSecond > 5695 && currentSecond <= 5700) || (currentSecond > 5705 && currentSecond <= 5710) || (currentSecond > 5715 && currentSecond <= 5720) || (currentSecond > 5725 && currentSecond <= 5730) || (currentSecond > 5735 && currentSecond <= 5740) || (currentSecond > 5745 && currentSecond <= 5750) || (currentSecond > 5770 && currentSecond <= 5820)) {
           xStrip.currentColor = xStrip.fullColor(38,181,253);
           xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if (currentSecond > 5820 && currentSecond <= 5880) {
           xStrip.currentColor = xStrip.fullColor(0,0,0);
           xStrip.setXRGBPattern(XRGB::DISPLAYCOL, LED::RGB);
        } else if (currentSecond > 5880 && currentSecond <= 6000) {
           xStrip.setXRGBPattern(XRGB::NONE, LED::RGB);
        }
    }
}

// -------------------------------- CALLBACK FUNCTIONS ----------------------------------------
void aCompletionCall() {
    
}

void firstStrobeCallback() { 
    xStrip.setXRGBPattern(XRGB::FLASH, LED::RANDOM, 8, 0.25, 1000);
}

void randomColorChangeCompletion() {
    addressStrip.addrCol1 = addressStrip.wheel(random(255));
    addressStrip.addrCol2 = addressStrip.wheel(addressStrip.addrCol1/2);
}

void completeScannerGoMultiScanner() {
    if (addressStrip.addrActivePattern != RGB_ADDR::MULTISCANNER) {
       // convert to multiscanner when one scanner thing complete
       addressStrip.multiScanner(addressStrip.fullColor(229,0,123), addressStrip.fullColor(38,181,253), 2, 50);
    }
}

void completeMultiScannerGoToColorWipe() {
    if (addressStrip.addrActivePattern != RGB_ADDR::COLOR_WIPE) {
       // convert to color wipe when multi scanner done
       addressStrip.colorWipe(addressStrip.fullColor(229,0,123), 60);
    }
}




