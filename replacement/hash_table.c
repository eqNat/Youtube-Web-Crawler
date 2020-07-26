#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "hash_table.h"

// Make sure this value is a power of two since we're using & instead of %
#define VIDEO_TABLE_WIDTH 4194304
#define CHANNEL_TABLE_WIDTH 524288

// Video ID
struct V_Node {
	struct V_Node* next;
	int64_t data;
};

// Channel ID
struct C_Node {
	struct C_Node* next;
	int64_t lhalf;
	int64_t rhalf;
};

pthread_mutex_t lock;

struct V_Node* v_table[VIDEO_TABLE_WIDTH] = { 0 };

// It may be over-engineered to use double pointers since this code is modified from my BST implementation
int64_t V_traverse_insert(struct V_Node** node, int64_t data)
{
	while (1) { // This loops if another thread steals its insertion spot
		while (*node) {
			if ((*node)->data == data)
				return 0;
			node = &(*node)->next;
		}
		pthread_mutex_lock(&lock);
		if (*node == NULL) { // make sure another thread didn't steal its spot
			*node = calloc(sizeof(struct V_Node), 1);
			(*node)->data = data;
			pthread_mutex_unlock(&lock);
			return 1;
		}
	pthread_mutex_unlock(&lock);
	}
}

struct C_Node* c_table[CHANNEL_TABLE_WIDTH] = { 0 };

int64_t C_traverse_insert(struct C_Node** node, int64_t lhalf, int64_t rhalf)
{
	while (1) { // This loops if another thread steals its insertion spot
		while (*node) {
			if ((*node)->lhalf == lhalf && (*node)->rhalf == rhalf)
				return 0;
			node = &(*node)->next;
		}
		pthread_mutex_lock(&lock);
		if (*node == NULL) { // make sure another thread didn't steal its spot
			*node = calloc(sizeof(struct C_Node), 1);
			(*node)->lhalf = lhalf;
			(*node)->rhalf = rhalf;
			pthread_mutex_unlock(&lock);
			return 1;
		}
	pthread_mutex_unlock(&lock);
	}
}

int64_t video_insert(int64_t data)
{
	return V_traverse_insert(&v_table[(VIDEO_TABLE_WIDTH - 1) & data], data);
}

int64_t channel_insert(int64_t lhalf, int64_t rhalf)
{
	return C_traverse_insert(&c_table[(CHANNEL_TABLE_WIDTH - 1) & rhalf], lhalf, rhalf);
}
