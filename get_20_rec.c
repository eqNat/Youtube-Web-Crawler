#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mycurl.h"
#include "url_conversion.h"
#include "get_20_rec.h"


int get_20_rec(uint64_t id, uint64_t* rec_ids)
{
	char url[11];
	lltourl(id, url);
	char* html_data = handle_url(url);
	char* p_char;
	char* url_rec;
	char insert;

	if (!(p_char = strstr(html_data, "content-link spf-link"))) {
		if (p_char = strstr(html_data, "content-link")) {
			fprintf(stderr, "**************************************************************************\n");
			fprintf(stderr, "This HTML contains the string \"content-link\" but no following \"spf-link\"\n");
			fprintf(stderr, "**************************************************************************\n");
		} else {
			fprintf(stderr, "id %s failed.\n", url);
			free(html_data);
			return 0;
		}
	}
	for (int i = 0; i < 20; i++) {
		if (!(p_char = strstr(p_char, "content-link spf-link"))) {
			for (int j = i; j < 20; j++)
				rec_ids[j] = 0;
			break;
		}
		for (url_rec = p_char; *url_rec != '?'; url_rec--)
		{ /* We're just decreasing 'url_rec' */ }
		url_rec += 3;

		rec_ids[i] = urltoll(url_rec);
		p_char++;
	}

		free(html_data);
	return 1;
}
