// A C program to demonstrate linked list based implementation of queue
// Code mostly from geeksforgeeks.com
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

// A linked list (LL) node to store a queue entry
struct QNode {
    uint64_t key;
    struct QNode* next;
};

// The queue, front stores the front node of LL and rear stores the
// last node of LL
struct Queue {
    struct QNode *front, *rear;
};

static uint64_t count = 0;

uint64_t getQueueCount()
{
	return count;
}

// A utility function to create a new linked list node.
struct QNode* newNode(uint64_t k)
{
    struct QNode* temp = calloc(sizeof(struct QNode), 1);
    temp->key = k;
    return temp;
}

// A utility function to create an empty queue
struct Queue* createQueue()
{
    struct Queue* q = calloc(sizeof(struct Queue), 1);
    return q;
}

pthread_mutex_t lock;

// The function to add a key k to q
void push(struct Queue* q, uint64_t k)
{

    // Create a new LL node
    struct QNode* temp = newNode(k);

	pthread_mutex_lock(&lock);

    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL) {
        q->front = q->rear = temp;
		pthread_mutex_unlock(&lock);
        return;
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
	count++;
	pthread_mutex_unlock(&lock);
}

// Function to remove a key from given queue q
uint64_t pop(struct Queue* q)
{
	pthread_mutex_lock(&lock);
    // If queue is empty, return NULL.
    if (q->front == NULL) {
		pthread_mutex_unlock(&lock);
        return 0;
	}

    // Store previous front and move front one node ahead
    struct QNode* temp = q->front;
	uint64_t temp_key = temp->key;

    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    free(temp);
	count--;
	pthread_mutex_unlock(&lock);
	return temp_key;
}
