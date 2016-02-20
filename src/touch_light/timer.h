#ifndef TIMER_H
#define TIMER_H

// Very basic software timer that supports a single timer callback at a time
class Timer {
public:
    Timer() {
        reset();
    }

    typedef void(*TimerFn)(void);

    // Calls cb every ms milliseconds.
    // If repeat is < 0, cb will be trigger infinitely.  Otherwise, it will be
    // triggered repeat times.
    void every(unsigned long ms, TimerFn cb, int repeat=-1);

    // Calls cb just one time after ms milliseconds.
    void once(unsigned long ms, TimerFn cb);

    // Resets the timer which cancels any current animation
    void reset();

    // Must be called regularly in loop().  Actually performs the callback.
    void process();

    // Delays the next timer callback by ms milliseconds.
    // Note: this is a non-blocking call.
    void delay(unsigned long ms);

    // Returns the current repeat value
    int repeat();

protected:
    unsigned long _interval;
    unsigned long _nextTime;
    TimerFn _callback;
    int _repeat;
};

#endif
