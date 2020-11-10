#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "chunker.h"

#define MIN(x, y) ((x) < (y)) ? x : y

void must_read_n(SSL* ssl, char* buf, int read_count)
{
    int i = 0; // number of bytes read
    do {
        int bytes = SSL_read(ssl, &buf[i], read_count - i);
        if (bytes < 1)
            fprintf(stderr, "status = %d: %s\n", bytes, strerror(errno)), exit(1);
        i += bytes;
    } while (i != read_count);
}

int must_read_until(SSL* ssl, char* buf, char delimiter)
{
    int i = 0;
    do {
        int bytes = SSL_read(ssl, &buf[i], 1);
        if (bytes < 1)
            fprintf(stderr, "status = %d: %s\n", bytes, strerror(errno)), exit(1);
    } while (buf[i++] != '\n');
    return i;
}

// This function may come in handy when I work with compressed data
__attribute__ ((unused))
int next_chunk(SSL* ssl, char** buf, int* buf_size)
{
    char hex_str[12] = { 0 };
    int bytes;
    int i = 0;

    do {
        if ((bytes = SSL_read(ssl, &hex_str[i], 1)) < 1)
            fprintf(stderr, "%s", strerror(errno)), exit(1);

    } while (hex_str[i++] != '\n');

    int chunk_size; // bytes to read
    sscanf(hex_str, "%x", &chunk_size);

    if (*buf_size < chunk_size) {
        if (*buf_size)
            free(buf);
        *buf = malloc(chunk_size);
        *buf_size = chunk_size;
    }

    i = 0; // number of bytes read
    do {
        bytes = SSL_read(ssl, &(*buf[i]), chunk_size - i);
        if (bytes < 1)
            fprintf(stderr, "status = %d: %s\n", bytes, strerror(errno)), exit(1);
        i += bytes;
    } while (i != chunk_size);
}

int check_header(struct Chunker *chunker, char* header)
{
    int i = 0;

    if (strncmp(header, "HTTP/1.1 2", 10)) {
        fprintf(stderr, "\n");
        for (int j = 0; j < i; j++)
            fprintf(stderr, "0x%x ", header[j]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Error: i = %d\n", i), exit(1);
    }

    do {
        SSL_read(chunker->ssl, &header[i], 1);
    } while (strncmp("\r\n\r\n", &header[i++ - 3], 4));
}


int chunk_read(struct Chunker *chunker, char* buf, int read_count)
{
    int i = 0;
    while (read_count) {
        char hex_str[4096] = { 0 };
        if (chunker->bytes_left == 0) {
            if (must_read_until(chunker->ssl, hex_str, '\n') == 17) {
                check_header(chunker, hex_str);
                must_read_until(chunker->ssl, hex_str, '\n');
            }

            sscanf(hex_str, "%x", &chunker->bytes_left);

            if (chunker->bytes_left == 0) {
                must_read_n(chunker->ssl, hex_str, 2);
                return i;
            }
        }
        int sub_count = MIN(chunker->bytes_left, read_count);
        int status = SSL_read(chunker->ssl, &buf[i], sub_count);
        if (status < 1)
            return status;
        i += status;
        chunker->bytes_left -= status;
        read_count -= status;
        if (chunker->bytes_left == 0)
            must_read_n(chunker->ssl, hex_str, 2);
    }
    return i;
}

/*
int main()
{
    struct Chunker chunker = {
        .f = open("yt.html", O_RDONLY),
        .bytes_left = 0
    };

    check_header(&chunker);

    char buf[100];
    int status;
    while ((status = https_read(&chunker, buf, 100)) > 0)
        write(1, buf, status);
    if (status < 0)
        fprintf(stderr, "Error: write returned %d\n", status);

    close(chunker.f);
}
*/
