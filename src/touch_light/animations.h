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

// Returns true if the animation is currently scheduled
bool isScheduled(Timer::TimerFn fn);

// Sets the LEDs to an RGB value (0-255, 0=off, 255=on)
void setColor(byte r, byte g, byte b);

// ===================
// Animation Functions
//
// Resets LEDs, stops timer
void reset();

// Slowly fades in LEDs, then switches to blue after a successful "long" press
void touchStart();

// Blinks blue
void touchEnd();

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

// modes
void mode1();
void mode2();
void mode3();
};

#endif
