//
// Bed sensor
//
// Arduino Pins:
//  2: NRF24 IRQ
//  9: NRF24 CE
// 10: NRF24 SS (CSN)
// 11: NRF24 MOSI
// 12: NRF24 MISO
// 13: NRF24 SCK
// A4: MPR121 SDA
// A5: MPR121 SCK
//
#include <MySensor.h>
#include "Adafruit_MPR121.h"

#define SENSOR_VERSION           "0.6"
#define DEBUG                    1
#define CHILD_OCCUPIED_ID        1
#define CHILD_PADS_ID            2

#define MPR121_ADDR     0x5A    // I2C address
#define NUM_PADS        4
#define SAMPLE_INTERVAL 1000    // ms to update values from MPR121
#define TRIGGER_LIMIT   2       // num of pads that must be triggered to count
#define UPDATE_TIMES    30      // if state hasn't changed send updates after this count
#define NUM_SAMPLES     20
#define LED_PIN         4

uint8_t occupied[NUM_PADS] = {0};
uint8_t totalTriggered = 0;
uint8_t bedOccupied = 0;
uint8_t updateCounter = 0;

uint8_t samples[NUM_PADS][NUM_SAMPLES];
uint8_t iter = 0;

uint8_t avg[NUM_PADS];
uint8_t lwm[NUM_PADS];


// MySensors
MySensor gw;
MyMessage msg;

Adafruit_MPR121 mpr121 = Adafruit_MPR121();

#if DEBUG
#define LOG_LN_DEBUG(x)  Serial.println(x)
#define LOG_DEBUG(x)  Serial.print(x)
#else
#define LOG_LN_DEBUG(x)
#define LOG_DEBUG(x)
#endif


void blinkLed(int times, int waitMs) {
    for (uint8_t i = 0; i < times; ++i) {
        gw.wait(waitMs);
        digitalWrite(LED_PIN, HIGH);
        gw.wait(waitMs);
        digitalWrite(LED_PIN, LOW);
    }
}

void receiveMessage(const MyMessage &message);
void sendLwmStatus();

// calibration must occur when the sensors are in the default state
void calibrate() {
    uint8_t hwm[NUM_PADS];

    digitalWrite(LED_PIN, HIGH);
    gw.wait(1000);
    digitalWrite(LED_PIN, LOW);
    bedOccupied = 0;
    sendOccupiedStatus();

    LOG_LN_DEBUG("Calibrating...");
    blinkLed(5, 500);
    blinkLed(2, 100);

    uint8_t led = 0;
    for (iter = 0; iter < NUM_SAMPLES; ++iter) {
        for (uint8_t i = 0; i < NUM_PADS; ++i) {
            uint8_t current = mpr121.filteredData(i);
            if (iter == 0) {
                avg[i] = current;
                lwm[i] = hwm[i] = current;
            }
            samples[i][iter] = current;

            if (current < lwm[i])
                lwm[i] = current;
            if (current > hwm[i])
                hwm[i] = current;
            LOG_DEBUG(lwm[i]); LOG_DEBUG("\t");
            LOG_DEBUG(hwm[i]); LOG_DEBUG("\t");
        }
        LOG_LN_DEBUG();
        gw.wait(1000);
        led = !led;
        digitalWrite(LED_PIN, led);
    }

    for (uint8_t i = 0; i < NUM_PADS; ++i) {
        avg[i] = ((hwm[i] - lwm[i]) / 2) + lwm[i];
        LOG_DEBUG(lwm[i]); LOG_DEBUG("\t");
        LOG_DEBUG(avg[i]); LOG_DEBUG("\t");
        LOG_DEBUG(hwm[i]);
        LOG_LN_DEBUG();
    }

    iter = 0;
    digitalWrite(LED_PIN, LOW);
    blinkLed(2, 100);
    sendSensorStatus();
    sendLwmStatus();
}


void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    gw.begin(receiveMessage, 16, false, 0);
    gw.sendSketchInfo("Bed Sensor", SENSOR_VERSION);
    gw.present(CHILD_OCCUPIED_ID, S_MOTION);
    // w/ 4 pads - 2-5 are avg, 6-9 are lwm
    for (uint8_t i = 0; i < NUM_PADS; i++) {
        gw.present(CHILD_PADS_ID + i, S_CUSTOM);
        gw.present(CHILD_PADS_ID + i + NUM_PADS, S_CUSTOM);
    }

    LOG_LN_DEBUG("Bed occupancy sensor");

    if (!mpr121.begin(MPR121_ADDR)) {
        Serial.println("MPR121 not found");
        while (1);
    }

    calibrate();

    LOG_LN_DEBUG("Ready.");
}


void loop() {
    uint8_t prevBedOccupied = bedOccupied;

    totalTriggered = 0;
    for (uint8_t i = 0; i < NUM_PADS; ++i) {
        uint8_t current = mpr121.filteredData(i);
        samples[i][iter] = current;

        uint8_t maxval = 0;
        uint8_t minval = 255;
        for (uint8_t j = 0; j < NUM_SAMPLES; ++j) {
            if (samples[i][j] < minval)
                minval = samples[i][j];
            else if (samples[i][j] > maxval)
                maxval = samples[i][j];
        }

        avg[i] = ((maxval - minval) / 2) + minval;
        occupied[i] = avg[i] < lwm[i] ? 1 : 0;
        totalTriggered += occupied[i];

        LOG_DEBUG(current); LOG_DEBUG("\t");
        LOG_DEBUG(avg[i]); LOG_DEBUG("\t");
        LOG_DEBUG(minval); LOG_DEBUG("\t");
        LOG_DEBUG(maxval); LOG_DEBUG("\t");
        LOG_DEBUG(occupied[i]); LOG_DEBUG("\t");
    }
    LOG_LN_DEBUG();

    bedOccupied = totalTriggered >= TRIGGER_LIMIT ? 1 : 0;

    if (updateCounter++ > UPDATE_TIMES) {
        sendOccupiedStatus();
        sendSensorStatus();
        updateCounter = 0;
    }
    else if (bedOccupied != prevBedOccupied) {
        sendOccupiedStatus();
    }

    iter += 1;
    if (iter >= NUM_SAMPLES)
        iter = 0;

    gw.wait(SAMPLE_INTERVAL);
}

void sendOccupiedStatus() {
    msg.setSensor(CHILD_OCCUPIED_ID);
    msg.setType(V_TRIPPED);
    gw.send(msg.set(bedOccupied));
}

void sendSensorStatus() {
    for (uint8_t i = 0; i < NUM_PADS; i++) {
        msg.setSensor(CHILD_PADS_ID + i);
        msg.setType(V_VAR1);
        gw.send(msg.set(avg[i]));
    }
}

void sendLwmStatus() {
    for (uint8_t i = 0; i < NUM_PADS; i++) {
        msg.setSensor(CHILD_PADS_ID + i + NUM_PADS);
        msg.setType(V_VAR1);
        gw.send(msg.set(lwm[i]));
    }
}

void receiveMessage(const MyMessage &message) {
    if (message.type == V_VAR1) {
        calibrate();
    }
}
