#pragma once
#include <stdint.h>

int64_t approximateSubs(char* string);

int64_t stringToInt64(const char *str);

char* encode64(uint64_t id, char* url);

uint64_t decode64(const char* url);
