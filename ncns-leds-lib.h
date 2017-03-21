/*****************************************************************
   Functions library for LEDs Driving
  
  /!\ This library MUST kown NUM_STRIPS and NUM_ZONES constants
 *****************************************************************/
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#ifndef NUM_STRIPS
  #error 'NUM_STRIPS' constants is not defined
#endif

#ifndef NUM_ZONES
  #error 'NUM_ZONES' constants is not defined
#endif

#ifndef PIN_SC1
  #error 'PIN_SC1' is not defined
#endif 
#ifndef PIN_SC2
  #error 'PIN_SC2' is not defined
#endif 
#ifndef PIN_SC3
  #error 'PIN_SC3' is not defined
#endif  
#ifndef PIN_SC4
  #error 'PIN_SC4' is not defined
#endif  

Adafruit_NeoPixel * STRIPS[NUM_STRIPS];

// Debounce on scenarios changes in ms
#define DEBOUNCE_CHOICE  50

// Mode for refreshing leds
#define STRIP_MODE 0
#define ZONE_MODE  1

// Choix du scénario
int choix = 0;
// False when current scénario is repeated for the second time
bool firstTime = true;
// Counter to set interruptibles Delays
elapsedMillis elapsedTime;
// Time to hold a scenario before, changing (to be compare to elapsedTime
uint16_t holdTime = 0;

// Total of leds all over the strips
int totalNumLeds = 0;

/*****************************************************************
   Binking function for onboard LED
 *****************************************************************/
void board_blinking(int freq) {
  #ifdef TEENSY_LED
    int myMillis = millis();
    myMillis %= 2 * freq;
    if (myMillis >= freq) {
      digitalWrite(TEENSY_LED, HIGH);
    } else {
      digitalWrite(TEENSY_LED, LOW);
    }
  #endif
}

/*****************************************************************
   Setting a strip to a specific color.
 *****************************************************************/
void setStripColor(Adafruit_NeoPixel * strip, int r, int g, int b) {
  for (int i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, r, g, b);
  }
}

/*****************************************************************
   Setting all Strips to a specific color.
 *****************************************************************/
void SetAllStripsToColor(int r, int g, int b) {
  // Tout à noir
  for (int i = 0; i < NUM_STRIPS; i++) {
    setStripColor(STRIPS[i], r, g, b);
  }
}

/*****************************************************************
   clampMap
 *****************************************************************/
float clampMap(float value, float inMin, float inMax, float outMin, float outMax) {
  return constrain(map(constrain(value,  inMin,  inMax), inMin, inMax, outMin, outMax), outMin, outMax);
}

/*****************************************************************
   Init d'un scénario
 *****************************************************************/
void initScenario() {
  if (Serial) {
    Serial.print("Scénario "); Serial.println(choix);
  }
  firstTime = true;
}

/*****************************************************************
   Inti d'un scénario
 *****************************************************************/
void read_choice() {
  int choix_prec = choix;

  if(digitalRead(PIN_SC1) == LOW) {
    choix = 1;
  }
  
  if(digitalRead(PIN_SC2) == LOW) {
    choix = 2;
  }
  
  if(digitalRead(PIN_SC3) == LOW) {
    choix = 3;
  }
  
  if(digitalRead(PIN_SC4) == LOW) {
    choix = 4;
  }

  if( (digitalRead(PIN_SC1) == HIGH) && 
      (digitalRead(PIN_SC2) == HIGH) && 
      (digitalRead(PIN_SC3) == HIGH) && 
      (digitalRead(PIN_SC4) == HIGH)
    ) {
      choix = 5;
  }
  if (choix_prec != choix) {
    initScenario();
  }
  // Debouncing contact
  delay(DEBOUNCE_CHOICE);
}

/*****************************************************************
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
 *****************************************************************/
uint32_t Wheel(byte WheelPos) {
  Adafruit_NeoPixel strip = Adafruit_NeoPixel();
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


/*****************************************************************
  Setting one pixel of the total.
  all the strips are views as only one big strip
 *****************************************************************/
void setOnePixelOfAll(float ratio, uint32_t c) {
  int idxStrip = 0;
  int tempTotalA = 0;
  int tempTotalB = 0;
  float idxPixel = ratio * (float)totalNumLeds;

  for (idxStrip = 0; idxStrip < NUM_STRIPS - 1; idxStrip++) {
    tempTotalA += STRIPS[idxStrip]->numPixels();
    if (tempTotalA > idxPixel) {
      // On sort
      break;
    }
    tempTotalB += STRIPS[idxStrip]->numPixels();
    STRIPS[idxStrip]->setPixelColor(idxPixel - tempTotalB, c);
  }
}


/*****************************************************************
  Showing all strips
 *****************************************************************/
void showAllStrips() {
  // Néopixel
  for (int i = 0; i < NUM_STRIPS; i++) {
    STRIPS[i]->show();
  }
}

/*****************************************************************
   Some Awsome Scenarios
 *****************************************************************/

/*****************************************************************
   Constrained rainbow between two colors
 *****************************************************************/
void constrainedRainbow(int color1, int color2, float period) {
  float timeRatio = fmod(millis(), period) / period;
  for (float i = 0; i < totalNumLeds; i++) {
    float toWheel = fmod(i + timeRatio * totalNumLeds, totalNumLeds) / totalNumLeds;
    // Rampe montante de 0->0.5 et descendante de 0.5 -> 1
    if (toWheel > 0.5) {
      toWheel = 1 - toWheel;
    }
    // Setting up between 0 and 1.
    toWheel *= 2;
    // sizing between 0 and 255 to have every variation of color
    toWheel *= 255;
    // Mapping between desired colors 
    toWheel = map(toWheel, 0, 255, color1, color2);
    setOnePixelOfAll(i / totalNumLeds, Wheel(toWheel));
  }
}

/*****************************************************************
  Switch one leds every n on all strip
 *****************************************************************/
void setColorOneLedEvery(int intervall, int r, int g, int b) {
  for ( int i = 0; i < NUM_STRIPS; i++) {
    for (int p = 0; p < STRIPS[i]->numPixels(); p++) {
      if (p % intervall == 0 ) {
        STRIPS[i]->setPixelColor(p, r, g, b);
      } else {
        STRIPS[i]->setPixelColor(p, 0, 0, 0);
      }
    }
  }
}