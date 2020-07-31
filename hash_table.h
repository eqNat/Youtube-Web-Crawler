#pragma once

#include <stdint.h>

int64_t video_insert(int64_t data);
int64_t channel_insert(int64_t lhalf, int64_t rhalf);

extern uint64_t v_table_count;
extern uint64_t c_table_count;
