#pragma

#include <stdint.h>

// Don't push in the value 0, since 0 is returned for 'pop' when the queue is empty
void push(int64_t data);

// returns 0 if queue is empty
// else, returns popped value
int64_t pop();

extern uint64_t Q_Count;
