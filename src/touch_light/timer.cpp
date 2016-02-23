#include <Arduino.h>
#include "timer.h"

void Timer::every(unsigned long ms, Timer::TimerFn cb, int repeat) {
    _interval = ms;
    _nextTime = millis() + ms;
    _callback = cb;
    _repeat = repeat;
}

void Timer::once(unsigned long ms, Timer::TimerFn cb) {
    every(ms, cb, 0);
}

void Timer::reset() {
    _callback = NULL;
    _interval = 0;
    _repeat = 0;
    _nextTime = 0;
}

void Timer::process() {
    if (_callback == NULL)
        return;

    unsigned long current = millis();
    if (current < _nextTime)
        return;

    _nextTime = current + _interval;
    Timer::TimerFn prev = _callback;
    _callback();

    // check to see if the callback changed itself
    if (prev != _callback)
        return;

    if (_repeat >= 0) {
        _repeat -= 1;
        if (_repeat < 0)
            reset();
    }
}

void Timer::delay(unsigned long ms) {
    _nextTime += ms;
}

int Timer::repeat() {
    return _repeat;
}

bool Timer::isActive() {
    return _callback != NULL;
}

Timer::TimerFn Timer::getCallback() {
    return _callback;
}
