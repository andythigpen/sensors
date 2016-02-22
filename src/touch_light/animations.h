#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "timer.h"

namespace animations {
extern Timer timer;
extern byte red;
extern byte green;
extern byte blue;

// Initializes the LED animations given the pin numbers for each RGB channel
bool init(byte led_r, byte led_g, byte led_b);

// Runs the animation callbacks, if necessary
void update();

// Returns true if there is currently an animation scheduled, false otherwise
bool isActive();

// ===================
// Animation Functions
//
// Resets LEDs, stops timer
void reset();

// Slowly fades in LEDs, then switches to green after a successful "long" press
void longPressStart();

// Blinks green for a "short" press
void longPressEnd();

// Blinks blue for a "short" press
void shortPress();

// Blinks red if a "long" press was started but not finished
void invalidTouch();

// Pulses LEDs white with an increasing delay
void slowingPulse();

// Simulates a sunrise fade-in effect
void sunrise();

// Flashes red forever
void error();

// Flashes green once
void success();
};

#endif
