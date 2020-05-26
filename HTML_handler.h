#pragma once

#include "queue.h"
#include "bst.h"

#define REC_COUNT 18

struct Row {
	uint64_t id;
	uint64_t recommendations[REC_COUNT];
};

void HTML_handler(char* html_data, uint64_t id);
