#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <pthread.h>

#include "HTML_handler.h"
#include "url_conversion.h"

static inline void mutex_write(struct Row* row);

void HTML_handler(char* html_data, uint64_t id)
{
	struct Row row = {
		.id = id
	};

	regex_t exp;
	char* cursor = html_data;
	int ret = regcomp(&exp, "[A-Za-z0-9_-]{11}\" class=\" content-link", REG_EXTENDED);
	regmatch_t matches;
	if (ret)
		printf("regcomp failed with %d\n", ret);

	for (int i = 0; i < REC_COUNT; i++)
		if (regexec(&exp, cursor, 1, &matches, 0) == 0) {
			row.recommendations[i] = urltoll(&cursor[matches.rm_so]);
			cursor = &cursor[matches.rm_eo];
			if (BST_insert(row.recommendations[i]))
				push(row.recommendations[i]);
		} else {
#ifdef LOGGING
				fprintf(stderr, "Innocuous error: Only %d recommendations: pushing back.\n", i);
#endif
				push(id);
				return;
		}
	mutex_write(&row);
}

pthread_mutex_t lock;
// I'm sure there is a better way of doing this
static inline void mutex_write(struct Row* row)
{
	int32_t size;
	pthread_mutex_lock(&lock);
	int32_t fd = open("youtube.bin", O_WRONLY | O_APPEND | O_CREAT, 0755);
	if ((size = write(fd, row, sizeof(struct Row))) == -1) {
		perror("write failed: ");
		exit(1);
	}
	close(fd);
	pthread_mutex_unlock(&lock);
}
