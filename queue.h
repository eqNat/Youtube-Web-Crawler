#pragma

#include <stdint.h>

struct Queue;

struct Queue* createQueue();

void push(struct Queue* q, uint64_t k);

uint64_t pop(struct Queue* q);

uint64_t getQueueCount();
