#pragma once

#define REC_COUNT 19
struct Row {
	uint64_t id;
	uint64_t recommendations[REC_COUNT];
};

void get_row(char* html_data, struct Row* row);
