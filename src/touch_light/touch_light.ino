//
// Touch Light Controller
// A box with three touch pads on top and RGB status leds
//
// Arduino Pins:
//  2: NRF24 IRQ
//  3: Gate of PNP MOSFET (Red channel)
//  5: Gate of PNP MOSFET (Green channel)
//  6: Gate of PNP MOSFET (Blue channel)
//  9: NRF24 CE
// 10: NRF24 SS (CSN)
// 11: NRF24 MOSI
// 12: NRF24 MISO
// 13: NRF24 SCK
// A4: MPR121 SDA
// A5: MPR121 SCK
//
#include <MySensor.h>
#include "touch.h"
#include "timer.h"
#include "animations.h"

#define SENSOR_VERSION  "0.1"
#define DEBUG
#define CHILD_ID        1

#define MPR121_ADDR     0x5A    // I2C address
#define LED_R           3       // PWM pin
#define LED_G           5       // PWM pin
#define LED_B           6       // PWM pin
#define TOTAL_PADS      3       // Total number of touch pads

// Scenes
#define NONE        0
#define BRIGHTEN    1
#define DIM         2
#define LIGHTS_ON   3
#define LIGHTS_OFF  4
#define SCRIPT1     5
#define SCRIPT2     6

// MySensors
MySensor gw;
MyMessage msg(CHILD_ID, V_SCENE_ON);

//TODO:
// * Modal UI
//      * By default commands control lights in bedroom
//      * Tapping 1 (middle) switches modes to another room; switch colors
//        for each mode
//      * After 5 seconds w/ no input, mode times out and goes back to default
// * Add gw.process() to the loop so that messages can be received
// * On receive wakeup:
//      * start sunrise routine where leds slowly fade up
// * On receive script start:
//      * pulse leds
// * On receive script done:
//      * stop pulsing leds

// Sends the scene to the gateway with up to 3 retries on failure
bool sendScene(int scene) {
    byte errCnt = 0;
    bool success = false;
    while (!success && errCnt++ < 3) {
        success = gw.send(msg.set(scene));
    }
    return success;
}

// called when a touch event starts
void onTouchStart(touch::TouchEvent &event) {
#ifdef DEBUG
    Serial.print("start | ");
    Serial.println(event.start);
#endif
    animations::longPressStart();
}

void onTouchEnd(touch::TouchEvent &event) {
#ifdef DEBUG
    Serial.print("end   | ");
    Serial.print(event.length);
    Serial.print("  pads=0x");
    Serial.println(event.pads, HEX);
#endif
}

void onLongTouch(touch::TouchEvent &event) {
    bool success = false;

    if (event.pads == ~(~0 << TOTAL_PADS))
        success = sendScene(SCRIPT2);
    else if (event.pads == _BV(0))
        success = sendScene(LIGHTS_ON);
    else if (event.pads == _BV(2))
        success = sendScene(LIGHTS_OFF);

#ifdef DEBUG
    Serial.print("long  | ");
    Serial.println(success);
#endif

    if (success)
        animations::longPressEnd();
    else
        animations::invalidTouch();
}

void onShortTouch(touch::TouchEvent &event) {
    bool success = false;

    if (event.pads == ~(~0 << TOTAL_PADS))
        success = sendScene(SCRIPT1);
    else if (event.pads == _BV(0))
        success = sendScene(BRIGHTEN);
    else if (event.pads == _BV(2))
        success = sendScene(DIM);

#ifdef DEBUG
    Serial.print("short | ");
    Serial.println(success);
#endif

    if (success)
        animations::shortPress();
    else
        animations::invalidTouch();
}

void onIgnoreTouch(touch::TouchEvent &event) {
#ifdef DEBUG
    Serial.print("ign   | ");
    Serial.println(event.length);
#endif
    animations::reset();
}

void onInvalidTouch(touch::TouchEvent &event) {
#ifdef DEBUG
    Serial.print("inv   | ");
    Serial.println(event.length);
#endif
    animations::invalidTouch();
}

void setupTouchEvents() {
    touch::onTouchStart = onTouchStart;
    touch::onTouchEnd = onTouchEnd;
    touch::onLongTouch = onLongTouch;
    touch::onShortTouch = onShortTouch;
    touch::onIgnoreTouch = onIgnoreTouch;
    touch::onInvalidTouch = onInvalidTouch;
}

#ifdef DEBUG
// used for testing purposes only
void readDebugCommands() {
    while (Serial.available()) {
        int cmd = Serial.read();
        switch (cmd) {
            case 'q': animations::red -= 5; break;
            case 'a': animations::red += 5; break;
            case 'w': animations::green -= 5; break;
            case 's': animations::green += 5; break;
            case 'e': animations::blue -= 5; break;
            case 'd': animations::blue += 5; break;
            case 'r': animations::reset(); break;
            case 'f': animations::red = animations::green = animations::blue = 0; break;
            case 't': animations::sunrise(); break;
        }
        analogWrite(LED_R, animations::red);
        analogWrite(LED_G, animations::green);
        analogWrite(LED_B, animations::blue);
        Serial.print(animations::red);
        Serial.print(' ');
        Serial.print(animations::green);
        Serial.print(' ');
        Serial.println(animations::blue);
    }
}
#endif

void setup() {
    // initialize LED pins first
    animations::init(LED_R, LED_G, LED_B);

    // MySensors will setup Serial...
    gw.begin();
    gw.sendSketchInfo("Touch Light Controller", SENSOR_VERSION);
    gw.present(CHILD_ID, S_SCENE_CONTROLLER);

    Serial.println("Starting v" SENSOR_VERSION);

    if (!touch::init(MPR121_ADDR, TOTAL_PADS)) {
        Serial.println("MPR121 not found!");
        animations::errorCondition();
        while (1)
            animations::update();
    }
    Serial.println("MPR121 found...");
    setupTouchEvents();
}

void loop() {
    touch::update();
    animations::update();

#ifdef DEBUG
    readDebugCommands();
#endif
}
