#pragma

#include <stdint.h>

struct Queue;

extern struct Queue global_Q;

// Don't enqueue in the value 0, since 0 is returned for 'dequeue' when the queue is empty
void enqueue(struct Queue *Q, int64_t data);

// returns 0 if queue is empty
// else, returns dequeueped value
int64_t dequeue(struct Queue *Q);

extern uint64_t Q_Count;
