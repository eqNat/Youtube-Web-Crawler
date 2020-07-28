%{
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sqlite3.h>

#include "json.h"
#include "conversions.h"
#include "panic.h"
#include "queue.h"
#include "hash_table.h"

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) result = SSL_read(ssl, buf, max_size);

extern void yyerror(const char *message);

SSL *ssl;
sqlite3 *db;
sqlite3_stmt *res;
const char sql_insert[] = "INSERT INTO videos VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

// This kludge is bad for multithreading.
// For now, I'm just trying to get everything working the way I want on a single thread.
void db_init()
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	if (sqlite3_open("youtube.db", &db) != SQLITE_OK) {
		sqlite3_close(db);
		PANIC("Cannot open database: %s", sqlite3_errmsg(db));
	}

}

_Bool db_opened = 0;

int64_t id;
int rec_count;
_Bool ratings_found;

%}

%option noyywrap
%option stack

%s YTINITIALDATA
%s YTINITIALPLAYERRESPONSE
%s PRIMARY_INFO
%s SECONDARY_RENDERER
%s SECONDARY_RESULTS
%s METADATA
%s COMPACT_VIDEO_RENDERER

%%
 rec_count = 0;
 ratings_found = 0;
 printf("\n");

 if (!db_opened) {
	db_init();
	db_opened++;
 }

 if (sqlite3_prepare_v2(db, sql_insert, -1, &res, NULL) != SQLITE_OK) {
 	sqlite3_close(db);
	PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));
 }

 int server;
 SSL_CTX *ctx;
 {//initialize
	if (!(id = pop())) {
		printf("queue empty\n");
		sqlite3_close(db);
		return 0;
	}

	sqlite3_bind_int64(res, 1, id);

	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
		PANIC("SSL_CTX_new() failed.");

	// socket()
	server = socket(AF_INET6, SOCK_STREAM, 0);
	if (server < 0)
		PANIC("socket() failed. (%d)", errno);

	{// connect()
		struct sockaddr_in6 server_address = {0};
		server_address.sin6_family = AF_INET6;
		server_address.sin6_port = htons(443);

		if (inet_pton(AF_INET6, "2607:f8b0:4000:812::200e", &server_address.sin6_addr) != 1)
			PANIC("inet_pton failed");

		if (connect(server, (struct sockaddr*)&server_address, sizeof(server_address)) == -1)
			PANIC("connect() failed. (%d)\n", errno);
	}

	ssl = SSL_new(ctx);
	if (!ctx)
		PANIC("SSL_new() failed.");

	SSL_set_fd(ssl, server);
	if (SSL_connect(ssl) == -1) {
		ERR_print_errors_fp(stderr);
		PANIC("SSL_connect() failed.");
	}

	char request[] = "GET /watch?v=########### HTTP/1.1\r\n"
	                 "Host: www.youtube.com:443\r\n"
	                 "Connection: close\r\n"
	                 "User-Agent: https_simple\r\n\r\n";
	
	encode64(id, request+13);
	printf("retrieving www.youtube.com/watch?v=%.11s\n", request+13);

	SSL_write(ssl, request, sizeof(request)-1);
 }

"window[\"ytInitialData\"] = " {
	yy_push_state(YTINITIALDATA);
}

<YTINITIALDATA>{
	"\"videoPrimaryInfoRenderer\"" {
		yy_push_state(PRIMARY_INFO);
	}
	"window[\"ytInitialPlayerResponse\"]" {
		yy_push_state(YTINITIALPLAYERRESPONSE);
	}
}

<YTINITIALPLAYERRESPONSE>{
	"\"reason\":{\"simpleText\":\"Private video\"}" {
		for (int i = 2; i < 26; i++)
			sqlite3_bind_null(res, i);

		int status;
		if ((status = sqlite3_step(res)) != SQLITE_DONE)
			PANIC("sqlite3_step returned %d", status);

		SSL_shutdown(ssl);
		close(server);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		printf("Private");
		return 1;
	}
}

<PRIMARY_INFO>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		printf("Title: %.*s\n", yyleng-25, yytext+26);
		sqlite3_bind_text(res, 2, yytext+26, yyleng-25, SQLITE_STATIC);
	}
	"\"viewCount\":{\"simpleText\":\""[0-9,]+ {
		sqlite3_bind_int64(res, 3, stringToInt64(yytext+27));
	}
	"\"tooltip\":\""[0-9,]+" / "[0-9,]+ {
		ratings_found = 1;
		yytext += 11;
		sqlite3_bind_int64(res, 4, stringToInt64(yytext));
		yytext = strstr(yytext, "/");
		sqlite3_bind_int64(res, 5, stringToInt64(yytext+2));
	}
	"\"videoSecondaryInfoRenderer\"" {
		if (!ratings_found) {
			sqlite3_bind_null(res, 4);
			sqlite3_bind_null(res, 5);
		}
		yy_pop_state();
		yy_push_state(SECONDARY_RENDERER);
	}
}

<SECONDARY_RENDERER>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		// memcpy(cRow.name, yytext+26, yyleng-25);
		// printf("Channel name: %s\n", cRow.name);
	}
	"\"url\":\"/channel/UC"[0-9a-zA-Z_-]{22} {
		sqlite3_bind_int64(res, 6, decode64(yytext+18));
		sqlite3_bind_int64(res, 7, decode64(yytext+29));
	}
	"\"subscriberCountText\":{\"runs\":[{\"text\":\""[^ ]+ {
		// cRow.subscribers = approximateSubs(yytext+40);
		// printf("subscribers (Three significant figures): %ld\n", cRow.subscribers);
	}
	"\"metadataRowContainer\"" {
		yy_push_state(METADATA);
	}
	"\"secondaryResults\"" {
		yy_pop_state();
		yy_push_state(SECONDARY_RESULTS);
	}
}

<METADATA>{
	"\"text\":\"Age-restricted video" {
		//printf("Age-restricted video\n");
		for (int i = 8; i < 26; i++)
			sqlite3_bind_null(res, i);

		int status;
		if ((status = sqlite3_step(res)) != SQLITE_DONE)
			PANIC("sqlite3_step returned %d", status);

		SSL_shutdown(ssl);
		close(server);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		printf("Age-restricted\n");
		return 1;
	}

	"\"collapsedItemCount\"" {
		yy_pop_state();
	}
}

<SECONDARY_RESULTS>{
	"\"compactVideoRenderer\"" {
		yy_push_state(COMPACT_VIDEO_RENDERER);
	}
}

<COMPACT_VIDEO_RENDERER>{
	"\"videoId\":\""[0-9a-zA-Z_-]{11} {
		int64_t rec = decode64(yytext+11);
		sqlite3_bind_int64(res, rec_count+8, rec);
		if (video_insert(rec)) {
			push(rec);
		}

		yy_pop_state();
		if (++rec_count == 18) {
			int status;
			if ((status = sqlite3_step(res)) != SQLITE_DONE)
				PANIC("sqlite3_step returned %d", status);
			sqlite3_finalize(res);

			SSL_shutdown(ssl);
			close(server);
			SSL_free(ssl);
			SSL_CTX_free(ctx);
			printf("hash count: %lu, queue count %lu\n", v_table_count, Q_Count);
			return 1;
		}
	}
}

"\"currentVideoEndpoint\""|"</body>" {
	char id_str[11];

	// debuging
	if (yytext[0] == '<')
		fprintf(stderr, "current video end point not found: ");
	if (rec_count == 0) {
		fprintf(stderr, "%.11s%", encode64(id, id_str));
	}
	fprintf(stderr, "\n");

	SSL_shutdown(ssl);
	close(server);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	push(id);
	printf("current video\n");
	return 1;
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
