#pragma once

#include <ChainableLED.h>


// Pass by reference to avoid copying the LED driver (copying would duplicate
// the internal heap buffer and free it twice when the temporary is destroyed).
void startupAnimation(ChainableLED& leds);
