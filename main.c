#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h> 

#include "stack.h"
#include "bst.h"

#include "url_conversion.h"
#include "mycurl.h"
#include "get_row.h"

struct T_Data {
	struct Stack_Node* stack_head;
	struct BST_Node* bst_head;
	uint64_t row_count;
};

static inline void mutex_write(struct Row* row);

void* runner(void* args)
{
	struct T_Data* t_data = (struct T_Data*) args;
	struct Row row;
	char* html_data;

	while (row.id = pop(&t_data->stack_head)) {
		char ascii_id[11];
		html_data = download_webpage(row.id);
		get_row(html_data, &row);
		if (row.recommendations[17] == 0) {
			push(&t_data->stack_head, row.id);
			printf("pushing %s back.\n", lltourl(row.id, ascii_id));
			free(html_data);
			continue;
		}
		for (int32_t i = 0; i < 18; i++) {
			if (row.recommendations[i] && BST_insert(&t_data->bst_head, row.recommendations[i]))
				push(&t_data->stack_head, row.recommendations[i]);
		}
		mutex_write(&row);
		free(html_data);
		t_data->row_count++;
		//if (t_data->row_count % 100 == 0)
			printf("rows = %lu, bst = %lu, stack = %lu\n", t_data->row_count, getBSTCount(), getStackCount());
	}
	pthread_exit(0);
}

#define THREAD_NUM 1

int main()
{
	struct T_Data t_data = {
		.stack_head = NULL,
		.bst_head = NULL,
		.row_count = 0
	};

	{// load the 'youtube_bin' file if it exists, else load default video id.
		char default_id[11] = "nX6SAH3w6UI";
		struct Row buffer;
		int32_t fd;
		int32_t size;
		if ((fd = open("youtube_bin", O_RDONLY, 0755)) != -1) {
			printf("Loading 'youtube_bin'...\n");
			// load primary keys
			while (size = read(fd, &buffer, sizeof(struct Row))) {
				if (size == -1) {
					perror("error reading file: ");
					exit(1);
				}
				if (!BST_insert(&t_data.bst_head, buffer.id)) {
					fprintf(stderr, "**ERROR**: Attempted to insert duplicate value %lx\n", buffer.id);
					exit(1);
				}
				t_data.row_count++;
			}
			lseek(fd, 0, SEEK_SET);
			// load foreign keys
			while (read(fd, &buffer, sizeof(struct Row)))
				for (int32_t i = 0; i < 18; i++) {
					if (BST_insert(&t_data.bst_head, buffer.recommendations[i]))
						push(&t_data.stack_head, buffer.recommendations[i]);
			}
			close(fd);
			printf("rows = %lu, bst = %lu, stack = %lu\n", t_data.row_count, getBSTCount(), getStackCount());
		} else {
			printf("No file found: using default id\n");
			BST_insert(&t_data.bst_head, urltoll(default_id));
			push(&t_data.stack_head, urltoll(default_id));
			t_data.row_count = 1;
		}
	}

	{// multithreading setup and execution
		pthread_t tids[THREAD_NUM];
		for (int32_t i = 0; i < THREAD_NUM; i++) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_create(&tids[i], &attr, runner, &t_data);
			while (getStackCount() < 10)
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
