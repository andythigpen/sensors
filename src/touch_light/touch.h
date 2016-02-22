#ifndef TOUCH_H
#define TOUCH_H

namespace touch {

// Touch event tracking
extern struct TouchEvent {
    // pad bit-fields
    uint16_t current = 0;   // pads that are currently touched
    uint16_t pads = 0;     // pads that have been touched since start
    uint16_t last = 0;      // pads that were touched in the last update loop

    // timers
    unsigned long start = 0;    // start of the touch event in millis
    unsigned long length = 0;   // length of the touch event in millis
} event;


// =============
// Configuration
//
// touches longer than this will be considered a "long" touch
extern int longTouchThreshold;
// touches shorter than this will be considered a "short" touch
extern int shortTouchThreshold;
// touches shorter than this will be ignored
extern int minTouchThreshold;

// ===============
// Event Callbacks
//
// global touch events
extern void (*onTouchStart)(TouchEvent &);
extern void (*onTouchEnd)(TouchEvent &);
extern void (*onLongTouch)(TouchEvent &);
extern void (*onShortTouch)(TouchEvent &);
extern void (*onIgnoreTouch)(TouchEvent &);
extern void (*onInvalidTouch)(TouchEvent &);

// single-pad touch events
extern void (*onTouchPadStart)(TouchEvent &, int pad);
extern void (*onTouchPadEnd)(TouchEvent &, int pad);

// ===============
// Methods
//
// Initializes the MPR121 module.  Must be called before any other methods.
// Returns true if successfully initialized, false otherwise.
bool init(int addr, byte pads=13);

// Reads the current state of the touch pads, calls event callbacks
void update();

};

#endif
