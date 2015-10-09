//
//  Motion/light sensor
//
#include <Arduino.h>
#include <SPI.h>
#include <MySensor.h>

// Config
#define DEBUG               1
#define SLEEP_TIME          300000
#define BATTERY_POWERED     1       // 0 = wall plug, 1 = battery

// PIR motion sensor
#define MOTION_INPUT_PIN    3
#define MOTION_INTERRUPT    (MOTION_INPUT_PIN - 2)
#define MOTION_CHILD_ID     1

// Light level sensor
#define LIGHT_ENABLE_PIN    4
#define LIGHT_ANALOG_PIN    A0      // analog pin A0 (not pin 0)
#define LIGHT_CHILD_ID      2
#define LIGHT_RESISTOR_ON   HIGH
#define LIGHT_RESISTOR_OFF  LOW

// Battery level
#define VBATMAX             3.1     // The voltage of New Batteries
#define VBATDROPOUT         1.8     // The battery dropout voltage

long readVcc();

MySensor gw;
MyMessage motionMsg(MOTION_CHILD_ID, V_TRIPPED);
MyMessage lightMsg(LIGHT_CHILD_ID, V_LIGHT_LEVEL);
bool timerExpired;

void setup() {
    // setup the sensor and communicate w/gateway
    gw.begin();
    gw.sendSketchInfo("Motion Sensor", "1.7");
    gw.present(MOTION_CHILD_ID, S_MOTION);
    gw.present(LIGHT_CHILD_ID, S_LIGHT_LEVEL);

    // setup the pins
    pinMode(MOTION_INPUT_PIN, INPUT);
    pinMode(LIGHT_ANALOG_PIN, INPUT);
#if BATTERY_POWERED
    pinMode(LIGHT_ENABLE_PIN, OUTPUT);
    digitalWrite(LIGHT_ENABLE_PIN, LIGHT_RESISTOR_OFF);
#endif

    // send initial state(s)
    gw.send(motionMsg.set(0));
    gw.send(lightMsg.set(0));

    // sleep for 30s to allow the PIR to stabilize
#if DEBUG
    Serial.println("Waiting for PIR to stabilize...");
#endif
    gw.sleep(30000);
#if DEBUG
    Serial.println("Ready.");
#endif
}

void loop() {
    bool tripped = readMotionSensor();
    int lightLevel = readLightSensor();
    long batteryLevel = readBatteryLevel();

    gw.send(motionMsg.set(tripped ? "1" : "0"));
    gw.send(lightMsg.set(lightLevel));
    if (batteryLevel >= 0) {
        gw.sendBatteryLevel(batteryLevel);
    }

#if DEBUG
    Serial.print("pir: ");
    Serial.print(tripped);
    Serial.print(" light: ");
    Serial.print(lightLevel);
    Serial.print(" battery: ");
    Serial.println(batteryLevel);
#endif

    // go to sleep for a bit to allow the motion sensor to settle
    if (timerExpired)
        gw.sleep(2000);

    timerExpired = gw.sleep(MOTION_INTERRUPT, CHANGE, SLEEP_TIME);
}

bool readMotionSensor() {
    return digitalRead(MOTION_INPUT_PIN) == HIGH;
}

int readLightSensor() {
#if BATTERY_POWERED
    // turn the resistor network "on" for the analog reading
    // the enable pin should be connected to the base of a transistor
    digitalWrite(LIGHT_ENABLE_PIN, LIGHT_RESISTOR_ON);
    delay(2);
#endif

    int lightLevel = analogRead(LIGHT_ANALOG_PIN);
    lightLevel = map(lightLevel, 0, 1023, 0, 100);
    lightLevel = constrain(lightLevel, 0, 100);

#if BATTERY_POWERED
    // turn it off again to save as much power as possible while sleeping
    digitalWrite(LIGHT_ENABLE_PIN, LIGHT_RESISTOR_OFF);
#endif

    return lightLevel;
}

long readBatteryLevel() {
#if BATTERY_POWERED
    long batteryLevel = ((readVcc() - (VBATDROPOUT * 1000)) / (((VBATMAX-VBATDROPOUT) * 10)));
    if (batteryLevel > 100)
        batteryLevel = 100;
    else if (batteryLevel < 0)
        batteryLevel = 0;
    return batteryLevel;
#else
    return -1;
#endif
}
