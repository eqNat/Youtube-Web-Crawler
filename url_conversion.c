#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "url_conversion.h"

// This is how I learned to convert a Youtube ID to a 64 bit integer:
// https://webapps.stackexchange.com/a/101153

char* lltourl(uint64_t id, char* url)
{
	uint8_t temp;
	for (int i = 0; i < 11; i++) {
		if (i == 0) {
			temp = (id & 0xF) << 2;
			id >>= 4;
		} else {
			temp = (uint64_t)(id % 64);
			id >>= 6;
		}
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
			perror("unknown character in lltourl()");
		url[10-i] = temp;
	}
	return url;
}

uint64_t urltoll(const char* url)
{
	uint64_t id = 0;
	uint64_t temp;
	if (url[5] == ':')
		url = &url[32];
	
	for (int i = 0; i < 11; i++) {
		temp = url[10 - i];
		// I should really refactor this
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
			perror("unkown character in urltoll()");
		temp = (i == 0) ? temp >> 2 : temp << i * 6 - 2;
		id |= temp;
	}
	return id;
}
