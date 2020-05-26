#pragma

#include <stdint.h>

struct Queue* queue;

struct Queue* createQueue();

void push(struct Queue* q, uint64_t k);

uint64_t pop(struct Queue* q);

uint64_t getQueueCount();
