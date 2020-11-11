#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/ssl.h>

struct Chunker {
    SSL *ssl;
    int bytes_left;
    _Bool end_of_page;
};

int check_header(struct Chunker* chunker, char* header);

int chunk_read(struct Chunker* chunker, char* buf, int read_count);

void init_history();

void write_history(char* buf, uint64_t size);

void dump_history();

