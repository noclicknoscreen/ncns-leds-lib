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

#ifdef TEST_MODE
char serialInput_prec = ' ';
char serialInput = ' ';
int testIdx = 0;
#endif

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
  // Tout à une couleur
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
   Lecture du choix d'un scénario
 *****************************************************************/
void read_choice() {
  int choix_prec = choix;

#ifdef TEST_MODE
  int knx1, knx2, knx3, knx4;

  if (TEST_MODE == 0) {
    while (Serial.available()) {
      serialInput = Serial.read();
      // Dropping \n
      if (serialInput == '\n') {
          serialInput_prec = serialInput;
      }
    } 
    if (serialInput_prec != serialInput) {
      switch (serialInput) {
        case '1':
        { 
          knx1 = LOW;
          knx2 = HIGH;
          knx3 = HIGH;
          knx4 = HIGH; 
          serialInput = serialInput_prec; 
          break;
        }
        case '2':
        { 
          knx1 = HIGH;
          knx2 = LOW;
          knx3 = HIGH;
          knx4 = HIGH; 
          serialInput = serialInput_prec; 
          break;
        }
        case '3':
        { 
          knx1 = HIGH;
          knx2 = HIGH;
          knx3 = LOW;
          knx4 = HIGH; 
          serialInput = serialInput_prec; 
          break;
        }
        case '4':
        { 
          knx1 = HIGH;
          knx2 = HIGH;
          knx3 = HIGH;
          knx4 = LOW; 
          serialInput = serialInput_prec; 
          break;
        }
      }
      serialInput_prec = serialInput; 
    }
  }
#endif

#ifndef TEST_MODE
  int knx1 = digitalRead(PIN_SC1); 
  int knx2 = digitalRead(PIN_SC2); 
  int knx3 = digitalRead(PIN_SC3); 
  int knx4 = digitalRead(PIN_SC4);
#endif

  if(knx1 == LOW) {
    choix = 1;
  }
  
  if(knx2 == LOW) {
    choix = 2;
  }
  
  if(knx3 == LOW) {
    choix = 3;
  }
  
  if(knx4 == LOW) {
    choix = 4;
  }

  if( (knx1 == HIGH) && 
      (knx2 == HIGH) && 
      (knx3 == HIGH) && 
      (knx4 == HIGH)
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
  }
  STRIPS[idxStrip]->setPixelColor(idxPixel - tempTotalB, c);
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
  Lightening on n leds with a sinusoidal brightness and chase it
  over all strips
 *****************************************************************/
void sinusoidalTheaterChase(int nb_leds, float period, int r, int g, int b) {
  float timeRatio = fmod(millis(), period) / period;
  float centerChasePix = fmod(timeRatio * totalNumLeds, totalNumLeds) / totalNumLeds;
  float b_factor = 0;
  Adafruit_NeoPixel strip = Adafruit_NeoPixel();

  for (float i = 0; i < totalNumLeds; i++) {
    if ( centerChasePix > (i - nb_leds)/totalNumLeds && centerChasePix < (i + nb_leds)/totalNumLeds) {
      if (i / totalNumLeds > centerChasePix) {
        b_factor = 1 - centerChasePix;
      } else {
        b_factor = centerChasePix;
      }
      b_factor = abs(sin(TWO_PI*b_factor));
      setOnePixelOfAll(i / totalNumLeds, strip.Color(int(float(r)*b_factor), int(float(g)*b_factor), int(float(b)*b_factor)));
    } else {
      setOnePixelOfAll(i / totalNumLeds, strip.Color(20, 20, 20));
    }
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

/*****************************************************************
   Color Wipe Strip
 *****************************************************************/
boolean colorWipeStrip(Adafruit_NeoPixel * strip, int total_time, int r, int g, int b) {
  int intervall_time = int(total_time / strip->numPixels());
  float timeRatio = fmod(millis(), intervall_time) / intervall_time;
  int last_pixel = timeRatio * strip->numPixels();
  strip->setPixelColor(last_pixel, r, g, b);
  strip->show();
  if (last_pixel == strip->numPixels() ){
    return true;
  }
  return false;
}


/*****************************************************************
   TEST MODE for Strips
   Send 'T' by Serial to view each strips blinking
 *****************************************************************/
#ifdef TEST_MODE
void testerZonesEtStrips() {
  if (TEST_MODE == 2) {
    char c = Serial.read();
    if (c == 'T') {
      testIdx++;
      if (testIdx >= NUM_STRIPS) {
        testIdx = 0;
      }
      Serial.print("============= Begin Test Strip #");
      Serial.print(testIdx);
      Serial.println(" ===============");
    }
    Serial.print("Testing Strip #"); Serial.print(testIdx); Serial.print(", ");
    Serial.print(STRIPS[testIdx]->numPixels());
    Serial.println(" leds");
    SetAllStripsToColor(0, 0, 0);
    showAllStrips();
    delay(500);
    setStripColor(STRIPS[testIdx], 255, 255, 0);
    showAllStrips();
    delay(500);
  }
}
#endif




