#pragma

#include <stdint.h>

// Don't enqueue in the value 0, since 0 is returned for 'dequeue' when the queue is empty
void enqueue(int64_t data);

// returns 0 if queue is empty
// else, returns dequeueped value
int64_t dequeue();

extern uint64_t Q_Count;
