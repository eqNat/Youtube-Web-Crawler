#pragma

#include <stdint.h>

// Don't push in the value 0, since 0 is returned for 'pop' when the queue is empty
void push(uint64_t data);

// returns 0 if queue is empty
// else, returns popped value
uint64_t pop();

uint64_t getQCount();
