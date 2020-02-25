#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h> 

#include "stack.h"
#include "bst.h"
#include "url_conversion.h"
#include "get_20_rec.h"

struct T_Data {
	struct Stack_Node* stack_head;
	struct BST_Node* bst_head;
	int bst_count;
	int stack_count;
	int loop_count;
};

pthread_mutex_t lock;

void* runner(void* args)
{
	struct T_Data* t_data = (struct T_Data*) args;
	
	uint64_t id;
	uint64_t video_recs[20];
	char url[11];
	FILE *f;

	pthread_mutex_lock(&lock);
	while (id = pop(&t_data->stack_head)) {
		t_data->stack_count--;
		pthread_mutex_unlock(&lock);
		if (get_20_rec(id, video_recs)) {
			pthread_mutex_lock(&lock);
			f = fopen("youtube_bin", "ab");
			fwrite(&id, sizeof(uint64_t), 1, f);
			fwrite(video_recs, sizeof(uint64_t), 20, f);
			fclose(f);

			for (int j = 0; j < 20; j++) {
				if (video_recs[j] && BST_insert(&t_data->bst_head, video_recs[j])) {
					push(&t_data->stack_head, video_recs[j]);
					t_data->bst_count++;
					t_data->stack_count++;
				}
			}
			t_data->loop_count++;
			if (t_data->loop_count % 1000 == 0)
				printf("loop %d, bst = %d, stack = %d\n", t_data->loop_count, t_data->bst_count, t_data->stack_count);
		} else {
			pthread_mutex_lock(&lock);
		}

	}
	pthread_mutex_unlock(&lock);
	pthread_exit(0);
}

#define THREAD_NUM 6

int main()
{
	struct T_Data t_data = {
		.stack_head = NULL,
		.bst_head = NULL,
		.bst_count = 0,
		.stack_count = 0,
		.loop_count = 0
	};

	{// load 'youtube_bin' if it exists, else load default video id.
		char default_id[11] = "nX6SAH3w6UI";
		uint64_t buffer[21];
		int fd;
		if (access("youtube_bin", F_OK) != -1) {
			printf("loading 'youtube_bin'...\n");
			fd = open("youtube_bin", O_RDONLY);
			while (read(fd, buffer, 168)) {
				if (!BST_insert(&t_data.bst_head, buffer[0]))
					fprintf(stderr, "ERROR: Atempted to insert duplicate values.\n");
				t_data.bst_count++;
			}
			lseek(fd, 0, SEEK_SET);

			while (read(fd, buffer, 168))
			for (int i = 1; i < 21; i++) {
				if (BST_insert(&t_data.bst_head, buffer[i])) {
					push(&t_data.stack_head, buffer[i]);
					t_data.stack_count++;
					t_data.bst_count++;
				}
			}
			close(fd);
			printf("loop %d, bst = %d, stack = %d\n", t_data.loop_count, t_data.bst_count, t_data.stack_count);
		} else {
			BST_insert(&t_data.bst_head, urltoll(default_id));
			push(&t_data.stack_head, urltoll(default_id));
			t_data.bst_count++;
			t_data.stack_count++;
		}
	}

	{// multithreading setup and execution
		pthread_t tids[THREAD_NUM];
		for (int i = 0; i < THREAD_NUM; i++) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_create(&tids[i], &attr, runner, &t_data);
			while (t_data.stack_count < 10)
			{ /* wait for first thread to push its first set of IDs */ }
			printf("thread %d in\n", i);
		}

		for (int i = 0; i < THREAD_NUM; i++) {
			pthread_join(tids[i], NULL);
		}
	}
}
