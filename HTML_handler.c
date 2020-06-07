#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "HTML_handler.h"
#include "base64.h"

static inline void mutex_write(struct Row* row);

void HTML_handler(char* html_data, uint64_t id)
{
	struct Row row = {
		.id = id
	};

	char* cursor = html_data;

	for (int i = 0; i < REC_COUNT; i++)
		if (cursor = strstr(cursor, ">\n    <a href=\"/watch?v=")) {
			row.recommendations[i] = urltoll(cursor++ + 24);
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
