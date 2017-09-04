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

#define SENSOR_VERSION  "0.2"
#define DEBUG           1
#define CHILD_ID        1

#define MPR121_ADDR     0x5A    // I2C address
#define NUM_PADS        4
#define SAMPLE_INTERVAL 1000    // ms to update values from MPR121
#define TRIGGER_LIMIT   2       // num of pads that must be triggered to count
#define UPDATE_TIMES    30      // if state hasn't changed send updates after this count


// values are specific to the sensors and were determined through experimentation
const float noise[NUM_PADS] = {4.0, 4.0, 2.5, 2.5};
const float lwm[NUM_PADS] = {3.4, 5.0, 2.5, 1.7};
const float hwm[NUM_PADS] = {1.5, 2.5, 1.0, 0.6};
float baseline[NUM_PADS];
float avg[NUM_PADS];
float peak[NUM_PADS];
uint8_t occupied[NUM_PADS] = {0};
uint8_t totalTriggered = 0;
uint8_t bedOccupied = 0;
uint8_t updateCounter = 0;

#define FAST_ALPHA       0.7
#define SLOW_ALPHA       0.1


// MySensors
MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);

Adafruit_MPR121 mpr121 = Adafruit_MPR121();

#if DEBUG
#define LOG_LN_DEBUG(x)  Serial.println(x)
#define LOG_DEBUG(x)  Serial.print(x)
#else
#define LOG_LN_DEBUG(x)
#define LOG_DEBUG(x)
#endif



float runningAvg(float current, float prev, float alpha) {
    return alpha * current + (1.0 - alpha) * prev;
}


void calibrate() {
    gw.sleep(5000);
    for (uint8_t cnt = 0; cnt < 10; ++cnt) {
        for (uint8_t i = 0; i < NUM_PADS; ++i) {
            uint8_t current = mpr121.filteredData(i);
            if (cnt == 0)
                baseline[i] = (float)current;
            else
                baseline[i] = runningAvg(current, baseline[i], FAST_ALPHA);

            avg[i] = baseline[i];
            LOG_DEBUG(baseline[i]); LOG_DEBUG("\t");
        }
        LOG_LN_DEBUG();
        gw.sleep(1000);
    }
}


void setup() {
    gw.begin(NULL, 16);
    gw.sendSketchInfo("Bed Sensor", SENSOR_VERSION);
    gw.present(CHILD_ID, S_MOTION);

    LOG_LN_DEBUG("Bed occupancy sensor");

    if (!mpr121.begin(MPR121_ADDR)) {
        Serial.println("MPR121 not found");
        while (1);
    }

    gw.sleep(1000);
    gw.send(msg.set(0));

    LOG_LN_DEBUG("Calibrating...");
    calibrate();

    LOG_LN_DEBUG("Ready.");
}


void loop() {
    for (uint8_t i = 0; i < NUM_PADS; ++i) {
        uint8_t current = mpr121.filteredData(i);
        float alpha = 0.0;

        if ((current > baseline[i] + noise[i]) ||
                (current < baseline[i] - noise[i])) {
            // ignore values > 2 times our noise band
            if (abs(baseline[i] - current) < 2 * noise[i])
                alpha = FAST_ALPHA;
        }
        else
            alpha = SLOW_ALPHA;

        // running average of sensor value
        avg[i] = runningAvg(current, avg[i], alpha);

        // peak detection
        if (avg[i] > peak[i])
            peak[i] = avg[i];

        // presence detection
        if (!occupied[i] && avg[i] < baseline[i] - lwm[i]) {
            occupied[i] = 1;
            totalTriggered += 1;
            baseline[i] = peak[i];
        }
        else if (occupied[i] && avg[i] > baseline[i] - hwm[i]) {
            occupied[i] = 0;
            totalTriggered -= 1;
            peak[i] = 0;
        }

        LOG_DEBUG(current); LOG_DEBUG("\t");
        LOG_DEBUG(avg[i]); LOG_DEBUG("\t");
        LOG_DEBUG(baseline[i]); LOG_DEBUG("\t");
        LOG_DEBUG(peak[i]); LOG_DEBUG("\t");
        LOG_DEBUG(occupied[i]); LOG_DEBUG("\t");
    }

    if (totalTriggered >= TRIGGER_LIMIT) {
        if (!bedOccupied || updateCounter++ > UPDATE_TIMES) {
            gw.send(msg.set(1));
            updateCounter = 0;
        }
        bedOccupied = 1;
    }
    else {
        if (bedOccupied || updateCounter++ > UPDATE_TIMES) {
            gw.send(msg.set(0));
            updateCounter = 0;
        }
        bedOccupied = 0;
    }

    LOG_LN_DEBUG();

    gw.sleep(SAMPLE_INTERVAL);
}
