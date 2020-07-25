#pragma once

#include <string.h>

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

extern FILE* yyin;
extern int yylineno;

int yylex();

struct ID_128 {	// primary key
	int64_t l_half;
	int64_t r_half;
};

struct Channel_Row {
	struct ID_128 id;
	char name[20];
	int64_t subscribers;
};

struct Video_Row {
	int64_t id;	// primary key
	char title[100];
	int64_t views;
	int64_t likes;
	int64_t dislikes;
	struct ID_128 *channel_fk; // foriegn key
	int64_t recommendations[18];
};
