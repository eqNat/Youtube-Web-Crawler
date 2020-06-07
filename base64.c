#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "base64.h"

// This is how I learned to convert a Youtube ID to a 64 bit integer:
// https://webapps.stackexchange.com/a/101153

char* encode64(uint64_t id, char* url)
{
	uint8_t temp = (id & 0xF) << 2;
	id >>= 4;
	for (int i = 0; i < 11; i++) {
		if (temp < 26)
			temp += 65;
		else if (temp < 52)
			temp += 71;
		else if (temp < 62)
			temp -= 4;
		else if (temp == 62)
			temp = '-';
		else if (temp == 63)
			temp = '_';
		else
			fprintf(stderr, "Error encode64: unknown character: %c\n", temp), exit(1);
		url[10-i] = temp;
		temp = (uint64_t)(id % 64);
		id >>= 6;
	}
	url[11] = 0;
	return url;
}

uint64_t decode64(const char* url)
{
	uint64_t id = 0;
	uint64_t temp;
	if (url[5] == ':')
		url = &url[32];
	
	for (int i = 0; i < 11; i++) {
		temp = url[10 - i];
		if (isupper(temp))
			temp -= 65;
		else if (islower(temp))
			temp -= 71;
		else if (isdigit(temp))
			temp += 4;
		else if (temp == '-')
			temp = 62;
		else if (temp == '_')
			temp = 63;
		else
			fprintf(stderr, "decode64: unknown character: %c\n", url[10 - i]);
		temp = (i) ? temp << i * 6 - 2: temp >> 2;
		id |= temp;
	}
	return id;
}
