#include <Arduino.h>
#include "touch.h"
#include "Adafruit_MPR121.h"

namespace touch {

Adafruit_MPR121 mpr121 = Adafruit_MPR121();

void (*onTouchStart)(TouchEvent &) = NULL;
void (*onTouchEnd)(TouchEvent &) = NULL;
void (*onLongTouch)(TouchEvent &) = NULL;
void (*onShortTouch)(TouchEvent &) = NULL;
void (*onIgnoreTouch)(TouchEvent &) = NULL;
void (*onInvalidTouch)(TouchEvent &) = NULL;

void (*onTouchPadStart)(TouchEvent &, int pad) = NULL;
void (*onTouchPadEnd)(TouchEvent &, int pad) = NULL;

struct TouchEvent event;

int longTouchThreshold = 3100;
int shortTouchThreshold = 500;
int minTouchThreshold = 90;
byte total_pads = 13;


void startTouch() {
    event.start = millis();
    if (onTouchStart)
        onTouchStart(event);
}

void endTouch() {
    bool longPress, shortPress;

    event.length = millis() - event.start;

    if (event.length < minTouchThreshold) {
        if (onIgnoreTouch)
            onIgnoreTouch(event);
        goto cleanup;
    }

    longPress = (event.length >= longTouchThreshold);
    shortPress = (event.length <= shortTouchThreshold);

    if (longPress && onLongTouch)
        onLongTouch(event);
    else if (shortPress && onShortTouch)
        onShortTouch(event);
    else if (onInvalidTouch) {
        onInvalidTouch(event);
        goto cleanup;
    }

    if (onTouchEnd)
        onTouchEnd(event);

cleanup:
    event.start = event.pads = event.length = 0;
}

bool init(int addr, byte pads) {
    total_pads = pads;
    return mpr121.begin(addr);
}

void update() {
    event.current = mpr121.touched();
    event.pads |= event.current;

    if (!event.start && event.current)
        startTouch();
    else if (event.start && !event.current)
        endTouch();

    if (onTouchPadStart || onTouchPadEnd) {
        for (uint8_t pad = 0; pad < total_pads; ++pad) {
            if ((event.current & _BV(pad)) && !(event.last & _BV(pad))) {
                if (onTouchPadStart)
                    onTouchPadStart(event, pad);
            }
            if (!(event.current & _BV(pad)) && (event.last & _BV(pad))) {
                if (onTouchPadEnd)
                    onTouchPadEnd(event, pad);
            }
        }
    }

    event.last = event.current;
}

};
