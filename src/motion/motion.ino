//
//  Motion/light sensor
//
#include <Arduino.h>
#include <SPI.h>
#include <MySensor.h>

// Config
#define SENSOR_VERSION      "1.9"
#define DEBUG               1
#define SLEEP_TIME          600000
#define BATTERY_POWERED     1       // 0 = wall plug, 1 = battery
#define HAS_REGULATOR       1       // 0 = no boost converter

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
#define EXTERNAL_VCC_PIN    A2
#define VBATMAX             3.1     // The voltage of New Batteries
#define VBATDROPOUT         1.8     // The battery dropout voltage

long readVcc();

MyTransportNRF24 transport(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_HIGH);
MySensor gw(transport);
MyMessage motionMsg(MOTION_CHILD_ID, V_TRIPPED);
MyMessage lightMsg(LIGHT_CHILD_ID, V_LIGHT_LEVEL);
bool interrupted = false;

void setup() {
    // setup the sensor and communicate w/gateway
    gw.begin();
    gw.sendSketchInfo("Motion Sensor", SENSOR_VERSION);
    gw.present(MOTION_CHILD_ID, S_MOTION);
    gw.present(LIGHT_CHILD_ID, S_LIGHT_LEVEL);

    // setup the pins
    pinMode(MOTION_INPUT_PIN, INPUT);
    pinMode(LIGHT_ANALOG_PIN, INPUT);

#if BATTERY_POWERED
    pinMode(LIGHT_ENABLE_PIN, OUTPUT);
    digitalWrite(LIGHT_ENABLE_PIN, LIGHT_RESISTOR_OFF);
    pinMode(EXTERNAL_VCC_PIN, INPUT);
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
    Serial.println("Motion Sensor v" SENSOR_VERSION);
    Serial.print("Battery powered: ");
    Serial.println(BATTERY_POWERED ? "yes" : "no");
    Serial.print("Has regulator: ");
    Serial.println(HAS_REGULATOR ? "yes" : "no");
    Serial.print("Sleep time: ");
    Serial.println(SLEEP_TIME);
#endif
}

void loop() {
    if (interrupted) {
        reportMotion();
    }
    else {
        reportLightLevel();
#if BATTERY_POWERED
        reportBatteryLevel();
#endif
        // ignore false positives of the motion interrupt
        gw.sleep(2000);
    }

    interrupted = gw.sleep(MOTION_INTERRUPT, CHANGE, SLEEP_TIME);
}

void reportMotion() {
    bool tripped = readMotionSensor();
    gw.send(motionMsg.set(tripped ? "1" : "0"));
#if DEBUG
    Serial.print("pir: ");
    Serial.println(tripped);
#endif
}

void reportLightLevel() {
    int lightLevel = readLightSensor();
    gw.send(lightMsg.set(lightLevel));
#if DEBUG
    Serial.print("light: ");
    Serial.println(lightLevel);
#endif
}

void reportBatteryLevel() {
    uint8_t batteryLevel = readBatteryLevel();
    gw.sendBatteryLevel(batteryLevel);
#if DEBUG
    Serial.print("battery: ");
    Serial.println(batteryLevel);
#endif
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

uint8_t readBatteryLevel() {
    long batteryLevel;
#if HAS_REGULATOR
    batteryLevel = analogRead(EXTERNAL_VCC_PIN);
    batteryLevel = map(batteryLevel, 594, 1023, 0, 100);
#else
    batteryLevel = ((readVcc() - (VBATDROPOUT * 1000)) / (((VBATMAX-VBATDROPOUT) * 10)));
#endif
    return constrain(batteryLevel, 0, 100);
}
