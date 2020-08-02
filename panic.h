#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PANIC(...) nc_panic(__FILE__, __LINE__, __VA_ARGS__)

// I don't know if it makes a difference to use the 'format' attribute if
// printf will validate the string anyways
__attribute__((noreturn, cold, format (printf, 3, 4)))
void nc_panic(const char* file_name, int line_number, const char* format, ...);
