#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h> 
#include <curl/curl.h>

#include "queue.h"
#include "bst.h"

#include "url_conversion.h"
#include "get_row.h"

#define ROW_COUNT (getBSTCount()-getQueueCount())
#define THREAD_NUM 36


struct call_back_args {
	char* HTML;
	uint32_t offset;
	uint32_t capacity; // Will reallocate into larger heaps if needed
};

static size_t write_data(char* data, size_t size, size_t nmemb, struct call_back_args* args)
{
	if (args->offset + nmemb > args->capacity) {
		args->capacity = args->offset + nmemb;
		args->HTML = realloc(args->HTML, args->capacity + 1);
	}

	memcpy(&args->HTML[args->offset], data, nmemb);
	args->offset += nmemb;

	args->HTML[args->offset] = '\0';

	return nmemb;
}

void* runner(void* no_args)
{
	uint64_t id;
	char full_url[43] = "https://www.youtube.com/watch?v=";
	struct call_back_args args = {
		.HTML = calloc(524288, 1),
		.capacity = 524287
	};

	CURL *curl;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &args);

	while (id = pop(queue))
	{
		args.offset = 0;
		char ascii_id[11];

		lltourl(id, &full_url[32]);
		curl_easy_setopt(curl, CURLOPT_URL, full_url);

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			fprintf(stderr, "ERROR: curl_easy_perform failed:\n %s \n", curl_easy_strerror(res)), exit(1);

		get_row(args.HTML, id);
		if (ROW_COUNT % 100 == 0)
			fprintf(stderr, "rows = %lu, bst = %lu, queue = %lu\n", ROW_COUNT, getBSTCount(), getQueueCount());
	}
	curl_easy_cleanup(curl);
	pthread_exit(0);
}

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
			fprintf(stderr, "Loading 'youtube.bin'...\n");
			// load primary keys
			while (size = read(fd, &buffer, sizeof(struct Row))) {
				if (size == -1) {
					perror("error reading file: ");
					exit(1);
				}
				if (!BST_insert(&bst_root, buffer.id)) {
					fprintf(stderr, "**ERROR**: Attempted to insert duplicate value %lx\n \
					This may be due to newer commits changing the macro value REC_COUNT found in get_row.h \
					Either revert REC_COUNT to previous value or remove the youtube.bin file\n", buffer.id);
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
			fprintf(stderr, "rows = %lu, bst = %lu, stack = %lu\n", ROW_COUNT, getBSTCount(), getQueueCount());
		} else {
			fprintf(stderr, "No file found: using default id\n");
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
			fprintf(stderr, "thread %d in\n", i+1);
		}

		for (int32_t i = 0; i < THREAD_NUM; i++) {
			pthread_join(tids[i], NULL);
		}
	}
}
