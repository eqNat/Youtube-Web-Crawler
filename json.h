#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


typedef void* yyscan_t;
struct flex_io;

int yylex(yyscan_t);
int yylex_destroy(yyscan_t);
int yylex_init_extra(struct flex_io *, yyscan_t *);
