#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "conversions.h"

// Youtube shows sub counts only to three significant digits.
int64_t approximateSubs(char* string)
{
	float multiplier;
	sscanf(string, "%f", &multiplier);
	char c = string[strlen(string)-1];

	if (isalpha(c)) {
		multiplier *= 1000;

		if (c == 'M')
			multiplier *= 1000;
	}
	return (uint64_t)multiplier;
}

// allows commas in string
int64_t stringToInt64(const char *str)
{
	int i = 0;
	int64_t to_return = 0;

	if (str[0] == '-')
		i++;
	
	if (!isdigit(str[i]))
		fprintf(stderr, "Error stringToInt64: Invalid input\n"), exit(1);
	
	do {
		to_return *= 10; // unnecessary to multiply on first loop.
		to_return += str[i] - 48;
		i++;
		if (str[i] == ',')
			i++;	
	} while (isdigit(str[i]));

	if (str[0] == '-')
		to_return *= -1;

	return to_return;
}

// This is how I learned to convert a Youtube ID to a 64 bit integer:
// https://webapps.stackexchange.com/a/101153

// The number after each function represents the bit length of its input/output.
// All functions deal with string IDs of base-64 (including decode128)
// It is a coincidence that encode64 and decode64 deal with base-64.

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
			fprintf(stderr, "Error decode64: unknown character: %c\n", url[10 - i]);
		temp = (i) ? temp << i * 6 - 2: temp >> 2;
		id |= temp;
	}
	return id;
}

__int128 decode128(const char* url)
{
	__int128 id = 0;
	uint64_t temp;

	for (int i = 0; i < 22; i++) {
		temp = url[21 - i];
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
			fprintf(stderr, "Error decode128: unknown character: %c\n", url[21 - i]);
		temp = (i) ? temp << i * 6 - 2: temp >> 2;
		id |= temp;
	}
	return id;
}
