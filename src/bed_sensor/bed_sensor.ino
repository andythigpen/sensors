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

#define SENSOR_VERSION  "0.1"
#define DEBUG           1
#define CHILD_ID        1

#define MPR121_ADDR     0x5A    // I2C address
#define NUM_PADS        6
#define THRESHOLD       6
#define COUNT_LIMIT     5       // consecutive times the sample must be above/below threshold
#define SAMPLE_INTERVAL 1000    // ms to update values from MPR121
#define UPDATE_TIMES    30      // if state hasn't changed send updates after this count

// MPR pad state
uint8_t previous[NUM_PADS];
uint8_t baseline[NUM_PADS];
//uint8_t counter[NUM_PADS] = {0};
uint8_t occupied[NUM_PADS] = {0};
uint8_t states[COUNT_LIMIT] = {0};
uint8_t bedOccupied = 0;
uint8_t updateCounter = 0;

// MySensors
MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);

Adafruit_MPR121 mpr121 = Adafruit_MPR121();


void setup() {
    gw.begin(NULL, 16);
    gw.sendSketchInfo("Bed Sensor", SENSOR_VERSION);
    gw.present(CHILD_ID, S_MOTION);

#if DEBUG
    Serial.println("Bed occupancy sensor");
#endif

    if (!mpr121.begin(MPR121_ADDR)) {
        Serial.println("MPR121 not found");
        while (1);
    }

    gw.sleep(1000);

    for (uint8_t i = 0; i < NUM_PADS; ++i) {
        previous[i] = baseline[i] = mpr121.filteredData(i);
    }

    gw.send(msg.set(0));

    Serial.println("Ready.");
}


void loop() {
    uint8_t totalTriggered = 0;
    for (uint8_t i = 0; i < NUM_PADS; ++i) {
        uint8_t current = mpr121.filteredData(i);

        if (previous[i] > current
                && previous[i] - current >= THRESHOLD
                && !occupied[i]) {
            baseline[i] = previous[i];
            occupied[i] = 1;
        }
        else {
            occupied[i] = current <= baseline[i] - THRESHOLD;
        }

        totalTriggered += occupied[i];
        previous[i] = current;

#if DEBUG
        Serial.print(current); Serial.print("\t");
        Serial.print(baseline[i]); Serial.print("\t");
        Serial.print(occupied[i]); Serial.print("\t");
#endif
    }

    for (uint8_t i = 0; i < COUNT_LIMIT - 1; ++i) {
        states[i] = states[i + 1];
    }
    states[COUNT_LIMIT - 1] = totalTriggered >= 1 ? 1 : 0;

    if (memchr(states, 0, COUNT_LIMIT) == NULL) {
        if (!bedOccupied || updateCounter++ > UPDATE_TIMES) {
            gw.send(msg.set(1));
            updateCounter = 0;
        }
        bedOccupied = 1;
    }
    else if (memchr(states, 1, COUNT_LIMIT) == NULL) {
        if (bedOccupied || updateCounter++ > UPDATE_TIMES) {
            gw.send(msg.set(0));
            updateCounter = 0;
        }
        bedOccupied = 0;
    }

#if DEBUG
    Serial.print(bedOccupied);
    Serial.println();
#endif

    gw.sleep(SAMPLE_INTERVAL);
}
