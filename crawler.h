#pragma once

#include <openssl/ssl.h>

typedef void* yyscan_t;

struct flex_io { // yyextra
	SSL *ssl;
	sqlite3 *db;
	sqlite3_stmt *video_stmt;
};

void crawler(struct flex_io *io, yyscan_t scanner, SSL_CTX *ctx);
