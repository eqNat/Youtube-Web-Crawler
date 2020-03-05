#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h> 

#include "queue.h"
#include "bst.h"

#include "url_conversion.h"
#include "mycurl.h"
#include "get_row.h"

#define ROW_COUNT (getBSTCount()-getQueueCount())

struct Queue* queue;
struct BST_Node* bst_root;

static inline void mutex_write(struct Row* row);

void* runner(void* args)
{
	struct Row row;
	char* html_data;

	while (row.id = pop(queue)) {
		char ascii_id[11];
		html_data = download_webpage(row.id);
		get_row(html_data, &row);
		if (row.recommendations[REC_COUNT-1] == 0) {
			push(queue, row.id);
			free(html_data);
			continue;
		}
		for (int32_t i = 0; i < REC_COUNT; i++) {
			if (row.recommendations[i] && BST_insert(&bst_root, row.recommendations[i]))
				push(queue, row.recommendations[i]);
		}
		mutex_write(&row);
		free(html_data);
		if (ROW_COUNT % 100 == 0)
			printf("rows = %lu, bst = %lu, queue = %lu\n", ROW_COUNT, getBSTCount(), getQueueCount());
	}
	pthread_exit(0);
}

#define THREAD_NUM 32

int main()
{
	queue = createQueue();
	bst_root = NULL;

	{// load the 'youtube.bin' file if it exists, else load default video id.
		char default_id[11] = "nX6SAH3w6UI";
		struct Row buffer;
		int32_t fd;
		int32_t size;
		if ((fd = open("youtube.bin", O_RDONLY, 0755)) != -1) {
			printf("Loading 'youtube.bin'...\n");
			// load primary keys
			while (size = read(fd, &buffer, sizeof(struct Row))) {
				if (size == -1) {
					perror("error reading file: ");
					exit(1);
				}
				if (!BST_insert(&bst_root, buffer.id)) {
					fprintf(stderr, "**ERROR**: Attempted to insert duplicate value %lx\n", buffer.id);
					exit(1);
				}
			}
			lseek(fd, 0, SEEK_SET);
			// load foreign keys
			while (read(fd, &buffer, sizeof(struct Row)))
				for (int32_t i = 0; i < REC_COUNT; i++) {
					if (BST_insert(&bst_root, buffer.recommendations[i]))
						push(queue, buffer.recommendations[i]);
			}
			close(fd);
			printf("rows = %lu, bst = %lu, stack = %lu\n", ROW_COUNT, getBSTCount(), getQueueCount());
		} else {
			printf("No file found: using default id\n");
			BST_insert(&bst_root, urltoll(default_id));
			push(queue, urltoll(default_id));
		}
	}

	{// multithreading setup and execution
		pthread_t tids[THREAD_NUM];
		for (int32_t i = 0; i < THREAD_NUM; i++) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_create(&tids[i], &attr, runner, NULL);
			while (getQueueCount() < 10)
			{ /* wait for first thread to push its first set of IDs */ }
			printf("thread %d in\n", i+1);
		}

		for (int32_t i = 0; i < THREAD_NUM; i++) {
			pthread_join(tids[i], NULL);
		}
	}
}

pthread_mutex_t lock;

static inline void mutex_write(struct Row* row)
{
	int32_t size;
	pthread_mutex_lock(&lock);
	int32_t fd = open("youtube_bin", O_WRONLY | O_APPEND | O_CREAT, 0755);
	if ((size = write(fd, row, sizeof(struct Row))) == -1) {
		perror("write failed: ");
		exit(1);
	}
	close(fd);
	pthread_mutex_unlock(&lock);

}
