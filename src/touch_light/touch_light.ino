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

// Scene commands
#define NONE        0
#define BRIGHTEN    1
#define DIM         2
#define LIGHTS_ON   3
#define LIGHTS_OFF  4
#define SCRIPT1     5
#define SCRIPT2     6
#define SUNRISE     7
#define MAX_SCENE   7   // this should always == the last scene

#define MAX_MODE    3

// MySensors
MySensor gw;
MyMessage msg(CHILD_ID, V_SCENE_ON);
bool responseReceived = false;
byte mode = 0;
Timer modeTimer;

// Sets the current "mode" for the touch commands
//TODO:
void setMode(byte m) {
    mode = m;
    if (mode >= MAX_MODE)
        mode = 0;   // wrap

    switch (mode) {
        case 0: animations::reset(); break;
        case 1: animations::mode1(); break;
        case 2: animations::mode2(); break;
        case 3: animations::mode3(); break;
    }

    // reset back to 0
    if (mode != 0) {
        modeTimer.once(5000, []() {
            mode = 0;
        });
    }
}

// Sends the scene to the gateway and optionally waits for a response
bool sendScene(int scene, bool blocking=true, int ms=2000) {
    gw.send(msg.set(scene));

    if (!blocking)
        return true;

    unsigned long long timeout = millis() + ms;

    responseReceived = false;
    while (!responseReceived && millis() < timeout) {
        // don't block animations while waiting for a response
        animations::update();
        gw.process();
    }

    return responseReceived;
}

// handles incoming messages from the MySensors gateway
void handleMessage(const MyMessage &msg) {
    if (mGetCommand(msg) != C_SET) {
        Serial.println("Unknown command");
        return;
    }
    if (msg.type != V_SCENE_ON) {
        Serial.println("Invalid type");
        return;
    }

    // this is a valid response message
    responseReceived = true;

    int scene = msg.getInt();
#ifdef DEBUG
    Serial.print("recv  | Received message: ");
    Serial.println(scene);
#endif
    switch (scene) {
    case BRIGHTEN:
    case DIM:
    case LIGHTS_OFF:
    case LIGHTS_ON:
        animations::success();
        break;

    case SCRIPT1:
    case SCRIPT2:
        if (animations::isScheduled(animations::slowingPulse))
            animations::reset();
        else
            animations::slowingPulse();
        break;

    case SUNRISE:
        if (animations::isScheduled(animations::sunrise))
            animations::reset();
        else
            animations::sunrise();
        break;

    default:
        Serial.println("Unknown scene");
    }

    // clear the current scene on the gateway
    sendScene(NONE, false);
}


// called when a touch event starts
void onTouchStart(touch::TouchEvent &event) {
#ifdef DEBUG
    Serial.print("start | ");
    Serial.println(event.start);
#endif
    animations::touchStart();
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

    animations::touchEnd();

    if (event.pads == ~(~0 << TOTAL_PADS))
        success = sendScene(SCRIPT2);
    else if (event.pads == _BV(0))
        success = sendScene(LIGHTS_ON);
    else if (event.pads == _BV(1))
        success = setMode(mode++);
    else if (event.pads == _BV(2))
        success = sendScene(LIGHTS_OFF);

#ifdef DEBUG
    Serial.print("long  | ");
    Serial.println(success);
#endif

    if (!success)
        animations::error();
}

void onShortTouch(touch::TouchEvent &event) {
    bool success = false;

    animations::touchEnd();

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

    if (!success)
        animations::error();
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
            case 'q': animations::red += 5; break;
            case 'a': animations::red -= 5; break;
            case 'w': animations::green += 5; break;
            case 's': animations::green -= 5; break;
            case 'e': animations::blue += 5; break;
            case 'd': animations::blue -= 5; break;
            case 'r': animations::reset(); break;
            case 'f': animations::red = animations::green = animations::blue = 0; break;
            case 't': animations::sunrise(); break;
        }
        animations::setColor(
            animations::red, animations::blue, animations::green);
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
    gw.begin(handleMessage);
    gw.sendSketchInfo("Touch Controller", SENSOR_VERSION);
    gw.present(CHILD_ID, S_SCENE_CONTROLLER);

    Serial.println("Starting v" SENSOR_VERSION);

    if (!touch::init(MPR121_ADDR, TOTAL_PADS)) {
        Serial.println("MPR121 not found!");
        while (1);
    }
    Serial.println("MPR121 found...");
    setupTouchEvents();
}

void loop() {
    touch::update();
    animations::update();
    gw.process();

#ifdef DEBUG
    readDebugCommands();
#endif
}
