#include <Arduino.h>
#include "animations.h"

namespace animations {

Timer timer;
byte red = 255, green = 255, blue = 255;

byte led_r = 3;
byte led_g = 5;
byte led_b = 6;

bool init(byte r, byte g, byte b) {
    led_r = r;
    led_g = g;
    led_b = b;

    // set mosfet pins to output, turn off initially
    pinMode(led_r, OUTPUT);
    pinMode(led_g, OUTPUT);
    pinMode(led_b, OUTPUT);
    digitalWrite(led_r, HIGH);
    digitalWrite(led_g, HIGH);
    digitalWrite(led_b, HIGH);

    return true;
}

void update() {
    timer.process();
}

void reset() {
    timer.once(0, []() {
        red = green = blue = 255;
        digitalWrite(led_r, HIGH);
        digitalWrite(led_g, HIGH);
        digitalWrite(led_b, HIGH);
    });
}

void longPressStart() {
    timer.every(10, []() {
        if (green == 0) {
            red = blue = 255;
            digitalWrite(led_r, HIGH);
            digitalWrite(led_g, LOW);
            digitalWrite(led_b, HIGH);
            return;
        }

        green = blue = red -= 1;

        analogWrite(led_r, red);
        analogWrite(led_g, green);
        analogWrite(led_b, blue);
    });
    timer.delay(500);
}

void longPressEnd() {
    timer.every(500, []() {
        red = blue = 255;
        digitalWrite(led_r, HIGH);
        digitalWrite(led_b, HIGH);
        // 2:off, 1:on, 0:off
        if (timer.repeat() == 1) {
            green = 0;
            digitalWrite(led_g, LOW);
        }
        else {
            green = 255;
            digitalWrite(led_g, HIGH);
        }
        /* reset(); */
    }, 2);
}

void shortPress() {
    timer.once(0, []() {
        red = green = 255;
        blue = 0;
        digitalWrite(led_r, HIGH);
        digitalWrite(led_g, HIGH);
        digitalWrite(led_b, LOW);
        reset();
        timer.delay(500);
    });
}

void invalidTouch() {
    timer.once(0, []() {
        blue = green = 255;
        red = 0;
        digitalWrite(led_r, LOW);
        digitalWrite(led_g, HIGH);
        digitalWrite(led_b, HIGH);
        reset();
        timer.delay(500);
    });
}

int currentDelay = 10;
bool fadeIn = true;
void slowingPulse() {
    currentDelay = 10;
    red = green = blue = 255;
    timer.every(10, []() {
        red = green = blue += fadeIn ? -1 : 1;

        analogWrite(led_r, red);
        analogWrite(led_g, green);
        analogWrite(led_b, blue);

        if (red == 0 || red == 255) {
            fadeIn = !fadeIn;
            currentDelay += 10;
            timer.delay(currentDelay);
        }
    });
}

void sunrise() {
    digitalWrite(led_r, HIGH);
    digitalWrite(led_g, HIGH);
    digitalWrite(led_b, HIGH);
    red = green = blue = 255;

    timer.every(1000, []() {
        analogWrite(led_r, --red);

        // add in a little green for orange-yellowish color
        int frame = timer.repeat();
        if (frame % 15 == 0)
            analogWrite(led_g, --green);
    }, 254);
}

void errorCondition() {
    red = blue = green = 255;
    digitalWrite(led_r, HIGH);
    digitalWrite(led_g, HIGH);
    digitalWrite(led_b, HIGH);

    timer.every(500, []() {
        if (red == 255) {
            red = 0;
            digitalWrite(led_r, LOW);
        }
        else {
            red = 255;
            digitalWrite(led_r, HIGH);
        }
    });
}

}
