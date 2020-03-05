#pragma once

struct Row {
	uint64_t id;
	uint64_t recommendations[19];
};

void get_row(char* html_data, struct Row* row);
