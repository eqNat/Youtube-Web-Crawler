#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>

#include "get_row.h"
#include "url_conversion.h"

void get_row(char* html_data, struct Row* row)
{
	regex_t exp;
	char* cursor = html_data;
	int ret = regcomp(&exp, "[A-Za-z0-9_-]{11}\" class=\" content-link", REG_EXTENDED);
	regmatch_t matches;
	if (ret)
		printf("regcomp failed with %d\n", ret);

		for (int i = 0; i < REC_COUNT; i++)
			if (regexec(&exp, cursor, 1, &matches, 0) == 0) {
				row->recommendations[i] = urltoll(&cursor[matches.rm_so]);
				cursor = &cursor[matches.rm_eo];
			} else {
				row->recommendations[i] = 0;
			}
}
