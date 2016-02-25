#ifndef MODE_H
#define MODE_H

namespace mode {

// Returns the current mode for the touch commands
byte get();
// Sets the current mode for the touch commands
bool set(byte);
// Increments to the next mode
bool next();

// displays the animation for the current mode
void display();

// resets the timeout for the current mode
void resetTimeout();

// runs the mode timer, reschedules animations if necessary
void update();

};

#endif
