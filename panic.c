#include "panic.h"

void nc_panic(const char* file_name, int line_number, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s:%d: panic: ", file_name, line_number);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    exit(1);
}
