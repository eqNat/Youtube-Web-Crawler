#pragma once

#include "queue.h"
#include "bst.h"

#define REC_COUNT 18

struct Queue* queue;
struct BST_Node* bst_root;

struct Row {
	uint64_t id;
	uint64_t recommendations[REC_COUNT];
};

void get_row(char* html_data, uint64_t id);
