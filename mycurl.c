#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "mycurl.h"

struct url_data {
    size_t size;
    char* data;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data)
{
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    data->size += (size * nmemb);

#ifdef DEBUG
    fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
#endif
    tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

    if(tmp) {
        data->data = tmp;
    } else {
        if(data->data) {
            free(data->data);
        }
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

char *handle_url(char* url)
{
    CURL *curl;

	char full_url[43] = "https://www.youtube.com/watch?v=";
	strcat(full_url, url);

    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* reasonable size initial buffer */
    if(NULL == data.data) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    data.data[0] = '\0';

    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		for (int attempt = 1; attempt <= 10000000; attempt++) {
        	res = curl_easy_perform(curl);
        	if (res == CURLE_OK)
				break;
//			if (attempt == 10)
                fprintf(stderr, "Attempt %d for %s failed: %s\n", attempt, url, curl_easy_strerror(res));
		}
        curl_easy_cleanup(curl);
    }
    return data.data;
}
