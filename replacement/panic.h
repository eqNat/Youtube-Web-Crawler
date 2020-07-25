#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PANIC(...) nc_panic(__FILE__, __LINE__, __VA_ARGS__)

void nc_panic(const char* file_name, int line_number, const char* format, ...);
