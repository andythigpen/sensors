//
// RGBW Neopixel light controller
//
#include <Encoder.h>
#include <Adafruit_NeoPixel.h>
#include <MySensor.h>


// Configuration
#define SENSOR_VERSION      "1.2"
#define DEBUG               1
#define NUM_TOP_LEDS        56
#define NUM_BOTTOM_LEDS     34
#define DEFAULT_BRIGHTNESS  255
#define MAX_MODE            4
#define TIMEOUT_DEBOUNCE    50
#define TIMEOUT_LONG_PRESS  300
#define TIMEOUT_MODE        5000
#define TIMEOUT_STATUS      20000


// Pin definitions
#define PIN_ENC_A       3
#define PIN_ENC_B       A1
#define PIN_LED_R       4
#define PIN_LED_G       5
#define PIN_LED_B       A2
#define PIN_ENC_BTN     A0
#define PIN_NEO1_DIN    6
#define PIN_NEO2_DIN    7


// Sensors
#define MIN_CHILD        1
#define CHILD_TOP_ID     1
#define CHILD_BOTTOM_ID  2
#define MAX_CHILD        3


Encoder myEnc(PIN_ENC_A, PIN_ENC_B);
Adafruit_NeoPixel topStrip = Adafruit_NeoPixel(
    NUM_TOP_LEDS, PIN_NEO1_DIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel bottomStrip = Adafruit_NeoPixel(
    NUM_BOTTOM_LEDS, PIN_NEO2_DIN, NEO_GRBW + NEO_KHZ800);
MySensor gw;
MyMessage msg;


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


// Modes
// 0 = brightness
// 1 = white
// 2 = red
// 3 = green
// 4 = blue
#define MODE_BRI  0
#define MODE_W    1
#define MODE_R    2
#define MODE_G    3
#define MODE_B    4


namespace timeout {
    long mode = 0;
    long debounce = 0;
    long longPress = 0;
    long status = 0;
    bool active = false;    // true when user is interacting w/ device

    void resetMode() {
        mode = millis() + TIMEOUT_MODE;
        active = true;
    }
};

namespace state {
    long encoder = 0;       // last position of the encoder
    bool button = LOW;      // last read of the button
    bool pressed = false;   // is a press event in progress
    bool longPress = false; // true when long press detected
    uint8_t mode = 0;       // current interface mode

    void setEncoder(long e) {
        encoder = e;
        myEnc.write(e);
    }

    void setMode(uint8_t m) {
        mode = m;
        if (mode > MAX_MODE)
            mode = 0;
#if DEBUG
        Serial.print("mode:");
        Serial.println(mode);
#endif
        switch (mode) {
            case 1:
                digitalWrite(PIN_LED_R, LOW);
                digitalWrite(PIN_LED_G, LOW);
                digitalWrite(PIN_LED_B, LOW);
                break;
            case 2:
                digitalWrite(PIN_LED_R, LOW);
                digitalWrite(PIN_LED_G, HIGH);
                digitalWrite(PIN_LED_B, HIGH);
                break;
            case 3:
                digitalWrite(PIN_LED_R, HIGH);
                digitalWrite(PIN_LED_G, LOW);
                digitalWrite(PIN_LED_B, HIGH);
                break;
            case 4:
                digitalWrite(PIN_LED_R, HIGH);
                digitalWrite(PIN_LED_G, HIGH);
                digitalWrite(PIN_LED_B, LOW);
                break;
            case 0:
            default:
                digitalWrite(PIN_LED_R, HIGH);
                digitalWrite(PIN_LED_G, HIGH);
                digitalWrite(PIN_LED_B, HIGH);
                break;
        }
    }

    void nextMode() {
        setMode(mode + 1);
    }
};

namespace lights {
    struct LightStrip {
        LightStrip(Adafruit_NeoPixel &s) : strip(s), color(0) {
            prevColor = strip.Color(0, 0, 0, 255);
            animation.color = strip.Color(0, 0, 0, 255);
            animation.bri = DEFAULT_BRIGHTNESS;
            animation.steps = 0;
            animation.duration = 1000000;
            //animation.duration = 0;
            animation.wait = 0;
            animation.prevFrame = 0;
        }
        uint32_t color;
        uint32_t prevColor;
        uint8_t prevBri;
        Adafruit_NeoPixel &strip;

        struct {
            uint32_t color;
            uint8_t bri;
            uint32_t steps;
            uint32_t duration;
            uint32_t wait;
            uint32_t prevFrame;
        } animation;

        void resetAnimation() {
            byte current, to, diff;

            animation.steps = 1;
            animation.prevFrame = 0;
            animation.wait = 0;

            // don't animate anything when interacting directly with the controller
            if (!timeout::active && animation.duration) {
                for (uint8_t i = 0; i < 4; ++i) {
                    current = *((byte *)&color + i);
                    to = *((byte *)&animation.color + i);
                    diff = current > to ? current - to : to - current;
                    animation.steps = max(animation.steps, diff);
#if DEBUG
                    Serial.print("i:");
                    Serial.print(i);
                    Serial.print(" current:");
                    Serial.print(current);
                    Serial.print(" to:");
                    Serial.print(to);
                    Serial.print(" diff:");
                    Serial.print(diff);
                    Serial.print(" steps:");
                    Serial.println(animation.steps);
#endif
                }

                current = strip.getBrightness();
                to = animation.bri;
                diff = current > to ? current - to : to - current;
                animation.steps = max(animation.steps, diff);
                animation.wait = animation.duration / animation.steps;
            }

#if DEBUG
            Serial.print("steps:");
            Serial.print(animation.steps);
            Serial.print(" wait:");
            Serial.print(animation.wait);
            Serial.print(" duration:");
            Serial.println(animation.duration);
#endif
        }

        void animate() {
            // do not use animations for direct interaction
            //if (timeout::active)
            //    animation.steps = 1;

            //if (timeout::active || (animation.steps && micros() - animation.prevFrame > animation.wait))
            if (animation.steps && micros() - animation.prevFrame > animation.wait)
                displayFrame();
        }

        void displayFrame() {
            byte *current, *to;

            animation.prevFrame = micros();
            animation.steps -= 1;

            if (animation.steps == 0) {
                color = animation.color;
                strip.setBrightness(animation.bri);
            }
            else {
                for (uint8_t i = 0; i < 4; ++i) {
                    current = (byte *)&color + i;
                    to = (byte *)&animation.color + i;

                    if (*current > *to)
                        *current -= 1;
                    else if (*current < *to)
                        *current += 1;
                }

                byte bri = strip.getBrightness();
                if (bri > animation.bri)
                    strip.setBrightness(bri - 1);
                else if (bri < animation.bri)
                    strip.setBrightness(bri + 1);
            }

            update();
            draw();
        }

        bool isOn() {
            return color != 0 && animation.bri != 0;
        }

        void getRGBW(char *rgbwStr) {
            sprintf(rgbwStr, "%.6X", color & 0xFFFFFF);
            sprintf(rgbwStr + 6, "%.2X", color >> 24);
        }

        void begin() {
            strip.setBrightness(DEFAULT_BRIGHTNESS);
            strip.begin();
            update();
            draw();
        }

        void update() {
#if 0
            Serial.print("update: ");
            Serial.print(strip.getPin());
            Serial.print(" color:");
            Serial.println(color, HEX);
#endif
            for (uint16_t i = 0; i < strip.numPixels(); ++i) {
                strip.setPixelColor(i, color);
            }
        }

        void toggle(uint8_t m) {
            if (get(m))
                set(m, 0);
            else
                set(m, 255);
#if DEBUG
            Serial.print("toggle: ");
            Serial.print(strip.getPin());
            Serial.print(" mode:");
            Serial.print(m);
            Serial.print(" val:");
            Serial.println(get(m));
#endif
            //update();
        }

        uint8_t get(uint8_t m) {
            switch (m) {
                case MODE_W: return ((uint8_t *)&color)[3];
                case MODE_R: return ((uint8_t *)&color)[2];
                case MODE_G: return ((uint8_t *)&color)[1];
                case MODE_B: return ((uint8_t *)&color)[0];
                case MODE_BRI: return strip.getBrightness();
                default: return 0;
            }
        }

        void set(uint8_t m, uint8_t val) {
            switch (m) {
                case MODE_W: ((uint8_t*)&animation.color)[3] = val; break;
                case MODE_R: ((uint8_t*)&animation.color)[2] = val; break;
                case MODE_G: ((uint8_t*)&animation.color)[1] = val; break;
                case MODE_B: ((uint8_t*)&animation.color)[0] = val; break;
                case MODE_BRI: animation.bri = val; break;
            }

            state::setEncoder(val);
#if DEBUG
            Serial.print("set: ");
            Serial.print(strip.getPin());
            Serial.print(" mode:");
            Serial.print(m);
            Serial.print(" val:");
            Serial.println(val);
#endif
            //update();
            resetAnimation();
        }

        void set(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
            animation.color = strip.Color(r, g, b, w);
            resetAnimation();
            //update();
        }

        void turnOn() {
            if (prevBri)
                animation.bri = prevBri;
            else
                animation.bri = 255;

            if (animation.color == 0)
                animation.color = strip.Color(0, 0, 0, 255);
            resetAnimation();
        }

        void turnOff() {
            prevBri = animation.bri;
            animation.bri = 0;
            resetAnimation();
        }

        void toggleSwitch(int val) {
            if (val)
                turnOn();
            else
                turnOff();
        }

        void draw() {
#if 0
            Serial.print("draw: ");
            Serial.println(strip.getPin());
#endif
            strip.show();
        }
    };
    LightStrip top(topStrip);
    LightStrip bottom(bottomStrip);

    void updateAll() {
        top.update();
        bottom.update();
    }

    void toggleAll(uint8_t m) {
        if (m == MODE_BRI) {
            if (lights::top.isOn()) {
                lights::top.turnOff();
                lights::bottom.turnOff();
            }
            else {
                lights::top.turnOn();
                lights::bottom.turnOn();
            }
        }
        else {
            lights::top.toggle(m);
            lights::bottom.toggle(m);
        }
    }

    void setAll(uint8_t m, uint8_t val) {
        lights::top.set(m, val);
        lights::bottom.set(m, val);
    }

    void drawAll() {
        top.draw();
        bottom.draw();
    }
};


void setup() {
    gw.begin(receiveMessage, 15);
    gw.sendSketchInfo("RGBW", SENSOR_VERSION);
    gw.present(CHILD_TOP_ID, S_RGBW_LIGHT);
    gw.present(CHILD_BOTTOM_ID, S_RGBW_LIGHT);

    sendStatus();

#if DEBUG
    Serial.println("RGBW Controller start");
#endif

    // setup encoder pins
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    pinMode(PIN_ENC_BTN, INPUT);

    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
    digitalWrite(PIN_LED_B, HIGH);

    lights::top.begin();
    lights::bottom.begin();
}


void loop() {
    readButton();
    readEncoder();

    if (timeout::active && millis() > timeout::mode)
        onTimeoutMode();

    if (millis() > timeout::status)
        onTimeoutStatus();

    lights::top.animate();
    lights::bottom.animate();

    gw.process();
}


// Called when interface timer expires
void onTimeoutMode() {
#if DEBUG
    Serial.println("timeout mode");
#endif
    state::setMode(0);
    state::setEncoder(lights::top.get(state::mode));
    sendStatus();
    timeout::active = false;
}

// Called when status timer expires
void onTimeoutStatus() {
#if DEBUG
    Serial.println("timeout status");
#endif
    sendStatus();
    timeout::status = millis() + TIMEOUT_STATUS;
}


// Reads the button state and performs an action on change
void readButton() {
    bool btn = digitalRead(PIN_ENC_BTN);

    // button press start
    if (btn == HIGH && state::button == LOW &&
        (millis()  > timeout::debounce)) {
#if DEBUG
        Serial.println("press start");
#endif
        timeout::debounce = millis() + TIMEOUT_DEBOUNCE;
        timeout::longPress = millis() + TIMEOUT_LONG_PRESS;
        state::pressed = true;
        state::longPress = false;
    }

    if (state::pressed && (millis() > timeout::debounce)) {
        timeout::resetMode();

        if (millis() > timeout::longPress && !state::longPress) {
            onLongPress();
            state::longPress = true;
        }

        if (btn == LOW && state::button == HIGH) {
#if DEBUG
            Serial.println("press end");
#endif
            if (!state::longPress) {
                onShortPress();
            }
            state::pressed = false;
            timeout::debounce = millis() + TIMEOUT_DEBOUNCE;
        }
    }
    state::button = btn;
}


// Called on short button press events
void onShortPress() {
#if DEBUG
    Serial.println("short press");
#endif
    lights::toggleAll(state::mode);
    lights::drawAll();
}


// Called on long button press events
void onLongPress() {
#if DEBUG
    Serial.println("long press");
#endif
    state::nextMode();
    state::setEncoder(lights::top.get(state::mode));
}


// Handles on change event for encoder
void onChangeEncoder() {
#if DEBUG
    Serial.print("encoder:");
    Serial.print(state::encoder);
    Serial.print(" mode:");
    Serial.println(state::mode);
#endif
    lights::setAll(state::mode, state::encoder);
    lights::updateAll();
    lights::drawAll();
}

// Reads the encoder value and performs an action on change
void readEncoder() {
    long pos = myEnc.read();
    if (pos == state::encoder)
        return;

    if (pos > 255) {
        pos = 255;
        myEnc.write(255);
    }
    else if (pos < 0) {
        pos = 0;
        myEnc.write(0);
    }
    state::encoder = pos;
    timeout::resetMode();
    onChangeEncoder();
}

void sendStatus() {
    char rgbwStr[9];
    uint8_t val;

    // top light status
    msg.setSensor(CHILD_TOP_ID);
    msg.setType(V_RGBW);
    lights::top.getRGBW(rgbwStr);
    gw.send(msg.set(rgbwStr));

    msg.setType(V_STATUS);
    gw.send(msg.set(lights::top.isOn()));

    msg.setType(V_PERCENTAGE);
    val = lights::top.get(MODE_BRI);
    gw.send(msg.set(map(val, 0, 255, 0, 100)));

    msg.setType(V_VAR2);
    gw.send(msg.set(timeout::active));

    // bottom light status
    msg.setSensor(CHILD_BOTTOM_ID);
    msg.setType(V_RGBW);
    lights::bottom.getRGBW(rgbwStr);
    gw.send(msg.set(rgbwStr));

    msg.setType(V_STATUS);
    gw.send(msg.set(lights::bottom.isOn()));

    msg.setType(V_PERCENTAGE);
    val = lights::bottom.get(MODE_BRI);
    gw.send(msg.set(map(val, 0, 255, 0, 100)));

    msg.setType(V_VAR2);
    gw.send(msg.set(timeout::active));

    timeout::status = millis() + TIMEOUT_STATUS;
}

void receiveMessage(const MyMessage &message) {
    if (message.type == V_RGBW) {
        char rgbwStr[9];
        String hexstring = message.getString();
        hexstring.toCharArray(rgbwStr, sizeof(rgbwStr));

        unsigned long value = strtoul(rgbwStr, NULL, 16);
        byte w = value & 0xFF;
        byte r = value >> 24;
        byte g = value >> 16 & 0xFF;
        byte b = value >> 8 & 0xFF;

        if (message.sensor == CHILD_TOP_ID) {
            lights::top.set(r, g, b, w);
            //lights::top.setAnimation(new lights::FadeAnimation(&lights::top, lights::top.strip.Color(r, g, b, w), lights::animationDuration));
        }
        else if (message.sensor == CHILD_BOTTOM_ID) {
            lights::bottom.set(r, g, b, w);
            //lights::bottom.setAnimation(new lights::FadeAnimation(&lights::bottom, lights::bottom.strip.Color(r, g, b, w), lights::animationDuration));
        }

#if DEBUG
        Serial.print("child:");
        Serial.print(message.sensor);
        Serial.print(" V_RGBW:");
        Serial.println(value, HEX);
#endif

        // send optimistic response:
        msg.setSensor(message.sensor);
        msg.setType(V_RGBW);
        gw.send(msg.set(hexstring.c_str()));
    }

    if (message.type == V_STATUS) {
        int value = message.getInt();
        if (message.sensor == CHILD_TOP_ID)
            lights::top.toggleSwitch(value);
        else if (message.sensor == CHILD_BOTTOM_ID)
            lights::bottom.toggleSwitch(value);

#if DEBUG
        Serial.print("child:");
        Serial.print(message.sensor);
        Serial.print(" V_STATUS:");
        Serial.println(value);
#endif

        // send optimistic response:
        msg.setSensor(message.sensor);
        msg.setType(V_STATUS);
        gw.send(msg.set(value));
    }

    if (message.type == V_PERCENTAGE) {
        int value = message.getInt();
        int mapped = map(value, 0, 100, 0, 255);
        if (message.sensor == CHILD_TOP_ID)
            lights::top.set(MODE_BRI, mapped);
        else if (message.sensor == CHILD_BOTTOM_ID)
            lights::bottom.set(MODE_BRI, mapped);
#if DEBUG
        Serial.print("child:");
        Serial.print(message.sensor);
        Serial.print(" V_PERCENTAGE:");
        Serial.println(value);
#endif

        // send optimistic response:
        msg.setSensor(message.sensor);
        msg.setType(V_PERCENTAGE);
        gw.send(msg.set(value));
    }

    if (message.type == V_VAR1) {
        uint32_t value = message.getULong();
        if (message.sensor == CHILD_TOP_ID)
            lights::top.animation.duration = value;
        else if (message.sensor == CHILD_BOTTOM_ID)
            lights::bottom.animation.duration = value;
#if DEBUG
        Serial.print("child:");
        Serial.print(message.sensor);
        Serial.print(" V_VAR1:");
        Serial.println(value);
#endif

        // send optimistic response:
        msg.setSensor(message.sensor);
        msg.setType(V_VAR1);
        gw.send(msg.set(value));
    }
}
