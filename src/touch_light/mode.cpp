#include <Arduino.h>
#include "mode.h"
#include "timer.h"
#include "animations.h"

#define MAX_MODE        3
#define MODE_TIMEOUT    5000

namespace mode {

byte current = 0;
Timer modeTimer;

void display() {
    switch (current) {
        case 1: animations::mode1(); break;
        case 2: animations::mode2(); break;
        case 3: animations::mode3(); break;
    }
}

byte get() {
    return current;
}

bool set(byte m) {
    current = m;
    // wrap to 1 skipping 0, the default mode
    if (current > MAX_MODE)
        current = 1;

#ifdef DEBUG
    Serial.print("mode  | ");
    Serial.println(current);
#endif

    animations::reset();
    display();

    // reset back to 0 after timeout
    if (current != 0) {
        modeTimer.once(MODE_TIMEOUT, []() {
            set(0);
        });
    }
    return true;
}

bool next() {
    return set(current + 1);
}

void resetTimeout() {
    modeTimer.delay(MODE_TIMEOUT);
}

void update() {
    modeTimer.process();
    // reschedule the mode animation if it is currently set and no other
    // animation is currently running
    if (!animations::isActive() && current != 0)
        display();
}

};
