#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "queue.h"

struct Q_Node {
	struct Q_Node* next;
	int64_t data;
};

struct Queue {
	struct Q_Node* front;
	struct Q_Node* back;
} global_Q = { NULL, NULL };

uint64_t Q_Count = 0;

pthread_mutex_t key;

// Don't enqueue in the value 0, since dequeue() returns 0 when the queue is empty
void enqueue(struct Queue *Q, int64_t data)
{
	if (data == 0)
		fprintf(stderr, "Error: Queue prohibits zero value to be enqueueed.\n"), exit(1);
	pthread_mutex_lock(&key);
	if (Q->front == NULL) {
		Q->back = calloc(sizeof(struct Q_Node), 1);
		Q->front = Q->back;
	} else {
		Q->back->next = calloc(sizeof(struct Q_Node), 1);
		Q->back = Q->back->next;
	}
	Q->back->data = data;

	Q_Count++;

	pthread_mutex_unlock(&key);
}

// returns 0 if queue is empty
// else, returns dequeued value
int64_t dequeue(struct Queue *Q)
{
	pthread_mutex_lock(&key);
	if (Q->front == NULL) {
		pthread_mutex_unlock(&key);
		return 0;
	}
	int64_t to_return = Q->front->data;
	struct Q_Node* to_delete = Q->front;
	if (Q->front == Q->back) {
		Q->front = NULL;
		Q->back = NULL;
	} else {
		Q->front = Q->front->next;
	}
	free(to_delete);

	Q_Count--;

	pthread_mutex_unlock(&key);
	return to_return;
}
