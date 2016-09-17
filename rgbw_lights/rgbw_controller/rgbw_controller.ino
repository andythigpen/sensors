//
// RGBW Neopixel light controller
//
#include <Encoder.h>
#include <Adafruit_NeoPixel.h>
//#define MY_DEBUG 1
#include <MySensor.h>


// Configuration
#define SENSOR_VERSION      "1.0"
#define DEBUG               1
#define NUM_LEDS            90
#define DEFAULT_BRIGHTNESS  255
#define MAX_MODE            3
#define TIMEOUT_DEBOUNCE    100
#define TIMEOUT_LONG_PRESS  750
#define TIMEOUT_MODE        3000


// Pin definitions
#define PIN_ENC_A       3
#define PIN_ENC_B       A1
#define PIN_LED_R       4
#define PIN_LED_G       5
#define PIN_LED_B       A2
#define PIN_ENC_BTN     A0
#define PIN_NEO_DIN     6


// Sensors
//#define LIGHTS_BOTTOM_RIGHT  1
//#define LIGHTS_BOTTOM_LEFT   2
//#define LIGHTS_TOP           3
#define CHILD_ID  1

Encoder myEnc(PIN_ENC_A, PIN_ENC_B);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN_NEO_DIN, NEO_GRBW + NEO_KHZ800);
MySensor gw;
MyMessage dimmerMsg(CHILD_ID, V_PERCENTAGE);
MyMessage lightMsg(CHILD_ID, V_STATUS);
MyMessage rgbMsg(CHILD_ID, V_RGB);


const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


// LED color/settings
// ordered wrgb because that's the order of "modes" (white most used)
uint8_t wrgb[] = {0, 0, 0, 0};
uint8_t prev_wrgb[] = {0, 0, 0, 0};
uint8_t brightness = DEFAULT_BRIGHTNESS;
bool onoff = false;


// Physical UI (encoder/button)
long oldPosition  = -999;
bool btnPressed = false;
bool btnState = HIGH;
bool longPressDone = false;
int mode = -1;
unsigned long lastModeUpdate = 0;
unsigned long lastBtnUpdate = 0;


// forward declarations;
void updateLEDs(uint8_t from=0, uint8_t to=0);
void updateButton();
void updateEncoder();



void setup() {
  gw.begin(incomingMessage, 15);
  gw.sendSketchInfo("RGBW", SENSOR_VERSION);
  gw.present(CHILD_ID, S_RGB_LIGHT);

  //gw.send(lightMsg.set(0));
  //gw.send(dimmerMsg.set(0));
  //gw.send(rgbMsg.set("000000"));
  sendCurrentState();

  // setup encoder LED pins
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_ENC_BTN, INPUT);

  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, HIGH);

  strip.setBrightness(brightness);
  strip.begin();
  strip.show();

  Serial.println("RGBW Controller start");
  setMode(-1);
  delay(50);
}


void loop() {
  updateButton();
  updateEncoder();  

  // timeout the current mode after TIMEOUT_MODE
  if (millis() - TIMEOUT_MODE > lastModeUpdate && mode >= 0) {
    setMode(-1);
  }

  gw.process();
}


void setMode(int newMode) {
  mode = newMode;
  if (mode > MAX_MODE) {
    mode = 0;
  }

#if DEBUG
  Serial.print("mode: ");
  Serial.println(mode);
#endif

  switch (mode) {
    case 0:
      digitalWrite(PIN_LED_R, LOW);
      digitalWrite(PIN_LED_G, LOW);
      digitalWrite(PIN_LED_B, LOW);
      break;
    case 1:
      digitalWrite(PIN_LED_R, LOW);
      digitalWrite(PIN_LED_G, HIGH);
      digitalWrite(PIN_LED_B, HIGH);
      break;
    case 2:
      digitalWrite(PIN_LED_R, HIGH);
      digitalWrite(PIN_LED_G, LOW);
      digitalWrite(PIN_LED_B, HIGH);
      break;
    case 3:
      digitalWrite(PIN_LED_R, HIGH);
      digitalWrite(PIN_LED_G, HIGH);
      digitalWrite(PIN_LED_B, LOW);
      break;
    default:
      digitalWrite(PIN_LED_R, HIGH);
      digitalWrite(PIN_LED_G, HIGH);
      digitalWrite(PIN_LED_B, HIGH);
      break;
  }
}

void resetLEDs() {
    wrgb[0] = wrgb[1] = wrgb[2] = wrgb[3] = 0;
    myEnc.write(0);
}


void updateButton() {
  bool currentState = digitalRead(PIN_ENC_BTN);

  if (currentState == HIGH && btnState == LOW &&
      (millis() - TIMEOUT_DEBOUNCE > lastBtnUpdate)) {
    Serial.println("press start");
    lastBtnUpdate = millis();
    btnPressed = true;
    longPressDone = false;
  }

  if (btnPressed && (millis() - TIMEOUT_DEBOUNCE > lastBtnUpdate)) {
    lastModeUpdate = millis();

    if (millis() - lastBtnUpdate > TIMEOUT_LONG_PRESS && !longPressDone) {
        //Serial.println("long press");
        // long press action == toggle current color
        //lastModeUpdate = millis();
        if (mode >= 0) {
          if (wrgb[mode] == 0) {
            wrgb[mode] = 255;
          }
          else {
            wrgb[mode] = 0;
          }
          myEnc.write(wrgb[mode]);
        }
        else {
          //wrgb[0] = wrgb[1] = wrgb[2] = wrgb[3] = 0;
          //myEnc.write(0);
          resetLEDs();
        }
        updateLEDs();
        sendCurrentState();
        longPressDone = true;
    }
    
    if (currentState == LOW && btnState == HIGH) {
      Serial.print("press end: ");
      if (millis() - lastBtnUpdate > TIMEOUT_LONG_PRESS) {
        Serial.println("long press");
      }
      else {
        Serial.println("short press");
        setMode(mode + 1);
        myEnc.write(wrgb[mode]);
      }
      btnPressed = false;
      lastBtnUpdate = millis();
    }
  }
  btnState = currentState;
}


void updateEncoder() {
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    
    if (newPosition > 255) {
      newPosition = 255;
      myEnc.write(255);
    }
    else if (newPosition < 0) {
      newPosition = 0;
      myEnc.write(0);
    }

    if (mode < 0) {
      if (!wrgb[0] && !wrgb[1] && !wrgb[2] && !wrgb[3]) {
        setMode(0);
        updateLEDBrightness(DEFAULT_BRIGHTNESS);
      }
      else {
        updateLEDBrightness(newPosition);
      }
    }
    else {
      wrgb[mode] = newPosition;
      updateLEDs();
    }
    lastModeUpdate = millis();

#if DEBUG
    Serial.print("mode: ");
    Serial.print(mode);
    Serial.print(", val: ");
    Serial.println(newPosition);
#endif

    //wrgb[mode] = newPosition;
    //updateLEDs();
    sendCurrentState();
  }
}


void updateLEDs(uint8_t from, uint8_t to) {
    if (to == 0) {
      to = strip.numPixels();
    }
    for (uint16_t i = from; i < to; ++i) {
      strip.setPixelColor(
        i, strip.Color(pgm_read_byte(&gamma8[wrgb[1]]),
                       pgm_read_byte(&gamma8[wrgb[2]]),
                       pgm_read_byte(&gamma8[wrgb[3]]),
                       pgm_read_byte(&gamma8[wrgb[0]])));
    }
    strip.show();
}


void updateLEDBrightness(uint8_t bri) {
  brightness = bri;
  strip.setBrightness(brightness);
  strip.show();
}


void sendCurrentState() {
  long value = (((long)wrgb[1] << 16 & 0xFF0000) |
                ((long)wrgb[2] << 8 & 0xFF00) |
                (wrgb[3] & 0xFF));
  uint8_t bri = map(brightness, 0, 255, 0, 100);
  uint8_t onoff = bri ? 1 : 0;
  //String rgbStr = String(value, HEX);
  char rgbStr[9];
  sprintf(rgbStr, "%.6X", value);
  
#if DEBUG
  Serial.print("rgb:");
  Serial.print(rgbStr);
  Serial.print(" bri:");
  Serial.print(bri);
  Serial.print(" status:");
  Serial.println(onoff);
#endif

  gw.send(lightMsg.set(onoff));
  gw.send(dimmerMsg.set(bri));
  gw.send(rgbMsg.set(rgbStr));
}


void incomingMessage(const MyMessage &message) {
  if (message.type == V_RGB) {
    char rgbStr[7];
    String hexstring = message.getString();
    hexstring.toCharArray(rgbStr, sizeof(rgbStr));

    long value = strtol(rgbStr, NULL, 16);
    wrgb[0] = 0;
    wrgb[1] = value >> 16;
    wrgb[2] = value >> 8 & 0xFF;
    wrgb[3] = value & 0xFF;
    updateLEDs();

#if DEBUG
    Serial.print("value:");
    Serial.println(value, HEX);
    Serial.print("r:");
    Serial.print(wrgb[1], HEX);
    Serial.print(" g:");
    Serial.print(wrgb[2], HEX);
    Serial.print(" b:");
    Serial.println(wrgb[3], HEX);
#endif

    //gw.send(rgbMsg.set(rgbStr));
  }

  //TODO: when support is added to Home Assistant
  //      see https://github.com/home-assistant/home-assistant/pull/3338
  //if (message.type == V_RGBW) {
  //}

  if (message.type == V_STATUS) {
    int value = message.getInt();
    //wrgb[0] = value ? 255 : 0; //TODO: keep track of prev
    if (value) {
      //wrgb = prev_wrgb;
      memcpy(wrgb, prev_wrgb, sizeof(wrgb));
      if (!wrgb[0] && !wrgb[1] && !wrgb[2] && !wrgb[3]) {
        setMode(0);
        updateLEDBrightness(DEFAULT_BRIGHTNESS);
        wrgb[0] = 255;
      }
    }
    else {
      memcpy(prev_wrgb, wrgb, sizeof(prev_wrgb));
      //wrbg[0] = wrgb[1] = wrgb[2] = wrgb[3] = 0;
      resetLEDs();
    }
    updateLEDs();
    //gw.send(lightMsg.set(value));
    //gw.send(dimmerMsg.set(value ? 100 : 0));
  }

  if (message.type == V_PERCENTAGE) {
    int value = message.getInt();
    int bri = map(value, 0, 100, 0, 255);

#if DEBUG
    Serial.print("percentage:");
    Serial.print(value);
    Serial.print(" -> ");
    Serial.println(bri);
#endif

    updateLEDBrightness(bri);
    //strip.setBrightness(brightness);
    //strip.show();
    //wrgb[0] = mapped;

    //updateLEDs();
    //gw.send(lightMsg.set(1));
    //gw.send(dimmerMsg.set(value));
  }

  sendCurrentState();
}
