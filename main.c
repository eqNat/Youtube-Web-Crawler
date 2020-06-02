#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h> 
#include <curl/curl.h>
#include <ctype.h>

#include "url_conversion.h"
#include "HTML_handler.h"
#include "callbacks_for_callback.h"

#define ROW_COUNT (getBSTCount()-getQCount())
#define THREAD_NUM 36

struct call_back_args {
	char* filtered_HTML; // contains only alphanumerics, hypens and underscores
	uint32_t offset;
	uint32_t max_offset; // Will reallocate into larger heaps if needed
};

static size_t write_data(char* data, size_t size, size_t nmemb, struct call_back_args* args)
{
	if (args->offset + nmemb > args->max_offset) {
		args->max_offset = args->offset + nmemb;
		args->filtered_HTML = realloc(args->filtered_HTML, args->max_offset + 1);
	}

//	memcpy(&args->filtered_HTML[args->offset], data, nmemb);
//	args->offset += nmemb;

	for (uint32_t i = 0; i < nmemb; i++)
		if (isalnum(data[i]) || data[i] == '-' || data[i] == '_')
			args->filtered_HTML[args->offset++] = data[i];

	args->filtered_HTML[args->offset] = '\0';

	return nmemb;
}

void* runner(void* no_args)
{
	uint64_t id;
	char full_url[43] = "https://www.youtube.com/watch?v=";
	struct call_back_args args = {
		.filtered_HTML = calloc(524288, 1),
		.max_offset = 524287
	};

	CURL *curl;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &args);
	CURLcode res;

	while (id = pop())
	{
		args.offset = 0;
		char ascii_id[11];

		lltourl(id, &full_url[32]);
		curl_easy_setopt(curl, CURLOPT_URL, full_url);

		if (res = curl_easy_perform(curl) != CURLE_OK)
			fprintf(stderr, "ERROR: curl_easy_perform failed:\n %s \n", curl_easy_strerror(res));

		HTML_handler(args.filtered_HTML, id);

		if (ROW_COUNT % 100 == 0)
			fprintf(stderr, "IDs processed = %lu, waiting = %lu, total = %lu\n",
			ROW_COUNT, getQCount(), getBSTCount());
	}
	curl_easy_cleanup(curl);
	pthread_exit(0);
}

int main()
{
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
				if (!BST_insert(buffer.id)) {
					fprintf(stderr, "**ERROR**: Attempted to insert duplicate value %lx\n \
					This may be due to a newer commit changing the macro value REC_COUNT found in HTML_handler.h \
					Either revert REC_COUNT to previous value or simply remove the youtube.bin file\n", buffer.id);
					exit(1);
				}
			}
			lseek(fd, 0, SEEK_SET);
			// load foreign keys
			while (read(fd, &buffer, sizeof(struct Row)))
				for (int32_t i = 0; i < REC_COUNT; i++) {
					if (BST_insert(buffer.recommendations[i]))
						push(buffer.recommendations[i]);
			}
			close(fd);
			fprintf(stderr, "youtube.bin rows (IDs processed) = %lu, queue count (waiting) = %lu, BST count (total) = %lu\n",
			        ROW_COUNT, getQCount(), getBSTCount());
		} else {
			fprintf(stderr, "No file found. Using default ID\n");
			BST_insert(urltoll(default_id));
			push(urltoll(default_id));
		}
	}

	{// multithreading setup and execution
		pthread_t tids[THREAD_NUM];
		for (int32_t i = 0; i < THREAD_NUM; i++) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_create(&tids[i], &attr, runner, NULL);
			fprintf(stderr, "thread %d in\n", i+1);
			while (getQCount() < 10)
			{ /* wait for first thread to push its first set of recommendations */ }
		}

		for (int32_t i = 0; i < THREAD_NUM; i++) {
			pthread_join(tids[i], NULL);
		}
	}
}
