#include <Arduino.h>
#include "animations.h"

namespace animations {

Timer timer;
byte red = 0, green = 0, blue = 0;

byte led_r = 3;
byte led_g = 5;
byte led_b = 6;

void setColor(byte r, byte g, byte b) {
    red = r;
    green = g;
    blue = b;

    // The LEDs are controlled by a P-channel mosfet which means that
    // HIGH is fully off and LOW is fully ON
    // This "reverses" the passed in colors so that an argument of 0 actually
    // means fully off and 255 is fully on.
    byte invr = ~r;
    byte invg = ~g;
    byte invb = ~b;

#ifdef DEBUG
    Serial.print("color | ");
    Serial.print(r);
    Serial.print(" ");
    Serial.print(g);
    Serial.print(" ");
    Serial.print(b);
    Serial.print(" : ");
    Serial.print(invr);
    Serial.print(" ");
    Serial.print(invg);
    Serial.print(" ");
    Serial.println(invb);
#endif

    // use digitalWrite so that pins are fully off or on
    if (invr == 255)
        digitalWrite(led_r, HIGH);
    else if (invr == 0)
        digitalWrite(led_r, LOW);
    else
        analogWrite(led_r, invr);

    if (invg == 255)
        digitalWrite(led_g, HIGH);
    else if (invg == 0)
        digitalWrite(led_g, LOW);
    else
        analogWrite(led_g, invg);

    if (invb == 255)
        digitalWrite(led_b, HIGH);
    else if (invb == 0)
        digitalWrite(led_b, LOW);
    else
        analogWrite(led_b, invb);
}

bool init(byte r, byte g, byte b) {
    led_r = r;
    led_g = g;
    led_b = b;

    // set mosfet pins to output, turn off initially
    pinMode(led_r, OUTPUT);
    pinMode(led_g, OUTPUT);
    pinMode(led_b, OUTPUT);
    setColor(0, 0, 0);

    return true;
}

void update() {
    timer.process();
}

bool isActive() {
    return timer.isActive();
}

bool isScheduled(Timer::TimerFn fn) {
    return timer.getCallback() == fn;
}

void reset() {
    timer.once(0, []() {
        setColor(0, 0, 0);
    });
}

void touchStart() {
    setColor(0, 0, 0);

    timer.every(10, []() {
        if (blue == 255) {
            setColor(0, 0, 255);
            timer.reset();
            return;
        }

        setColor(++red, ++green, ++blue);
    });
    timer.delay(500);
}

void touchEnd() {
    setColor(0, 0, 255);

    // fade out over roughly 2 seconds
    timer.every(8, []() {
        if (blue == 0) {
            reset();
            return;
        }
        setColor(red, green, --blue);
    });
}

void invalidTouch() {
    timer.once(0, []() {
        setColor(255, 0, 0);
        reset();
        timer.delay(500);
    });
}

int currentDelay = 10;
bool fadeIn = true;
void slowingPulse() {
    currentDelay = 10;
    fadeIn = true;
    setColor(0, 0, 0);
    timer.every(10, []() {
        red = green = blue += fadeIn ? 1 : -1;

        setColor(red, green, blue);

        if (red == 0 || red == 255) {
            fadeIn = !fadeIn;
            currentDelay += 10;
            timer.delay(currentDelay);
        }
    });
}

void sunrise() {
    setColor(0, 0, 0);

    timer.every(1000, []() {
        // add in a little green for orange-yellowish color
        int frame = timer.repeat();
        if (frame % 15 == 0)
            green++;

        setColor(++red, green, blue);
    }, 254);
}

void error() {
    setColor(255, 0, 0);

    timer.every(250, []() {
        // 2:off, 1:on, 0:off
        if (timer.repeat() == 1) {
            setColor(255, 0, 0);
        }
        else {
            setColor(0, 0, 0);
        }
    }, 2);
}

void success() {
    setColor(0, 255, 0);

    timer.every(10, []() {
        if (green == 0) {
            reset();
            return;
        }
        setColor(0, --green, 0);
    });
}

void mode1() {
    setColor(255, 0, 255);
}

void mode2() {
    setColor(255, 255, 0);
}

void mode3() {
    setColor(0, 255, 255);
}

}
