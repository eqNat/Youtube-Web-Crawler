%{
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sqlite3.h>

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

#include "json.h"
#include "conversions.h"
#include "panic.h"
#include "queue.h"
#include "hash_table.h"

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) result = SSL_read(ssl, buf, max_size);

extern void yyerror(const char *message);

const char sql_video_insert[] = "INSERT INTO videos VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
const char sql_channel_insert[] = "INSERT INTO channels VALUES(?,?,?,?);";

_Thread_local SSL *ssl;
_Thread_local SSL_CTX *ctx;
_Thread_local sqlite3 *db = NULL;
_Thread_local sqlite3_stmt *video_stmt;
_Thread_local sqlite3_stmt *channel_stmt;
_Thread_local char channel_name[256];
_Thread_local char request[] =
	"GET /watch?v=########### HTTP/1.1\r\n"
	"Host: www.youtube.com:443\r\n"
	"Connection: close\r\n"
	"User-Agent: https_simple\r\n\r\n";
_Thread_local int server;
_Thread_local char title[512];

void db_init()
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	if (sqlite3_open("youtube.db", &db) != SQLITE_OK) {
		sqlite3_close(db);
		PANIC("Cannot open database: %s", sqlite3_errmsg(db));
	}
	int status;
	if ((status = sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL)) != SQLITE_OK)
		PANIC("PRAGMA failed: sqlite3_exec returned %d", status);
	
	sqlite3_busy_timeout(db, 100);
}

_Thread_local int64_t id;
_Thread_local int rec_count;
_Thread_local _Bool ratings_found;
_Thread_local int i;

%}

%option noyywrap stack reentrant

%s YTINITIALDATA
%s YTINITIALPLAYERRESPONSE
%s PRIMARY_INFO
%s SECONDARY_RENDERER
%s SUB_COUNT
%s SECONDARY_RESULTS
%s METADATA
%s COMPACT_VIDEO_RENDERER

%%
 rec_count = 0;
 ratings_found = 0;

 if (!db)
	db_init();

 if (sqlite3_prepare_v2(db, sql_video_insert, -1, &video_stmt, NULL) != SQLITE_OK) {
 	sqlite3_close(db);
	PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));
 }

 {//initialize
	if (!(id = pop())) {
		printf("queue empty\n");
		sqlite3_close(db);
		SSL_shutdown(ssl);
		close(server);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		return 0;
	}

	sqlite3_bind_int64(video_stmt, 1, id);

	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
		PANIC("SSL_CTX_new() failed.");

	// socket()
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0)
		PANIC("socket() failed. (%d)", errno);

	{// connect()
		struct sockaddr_in server_address = {0};
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(443);

		if (inet_pton(AF_INET, "172.217.1.238", &server_address.sin_addr) != 1)
			PANIC("inet_pton failed");

		for (i = 0; connect(server, (struct sockaddr*)&server_address, sizeof(server_address)) == -1; i++)
			if (i == 5)
				PANIC("connect() failed. (%d)\n", errno);
	}

	ssl = SSL_new(ctx);
	if (!ctx)
		PANIC("SSL_new() failed.");

	SSL_set_fd(ssl, server);
	for (i = 0; SSL_connect(ssl) == -1; i++) {
		if (i == 5) {
			push(id);
			close(server);
			SSL_CTX_free(ctx);
			return 1;
		}
		//ERR_print_errors_fp(stderr);
		//PANIC("SSL_connect() failed: %d", errno);
	}

	
	encode64(id, request+13);

	SSL_write(ssl, request, sizeof(request)-1);
 }

"window[\"ytInitialData\"] = " {
	yy_push_state(YTINITIALDATA, yyscanner);
}

<YTINITIALDATA>{
	"\"videoPrimaryInfoRenderer\"" {
		yy_push_state(PRIMARY_INFO, yyscanner);
	}
	"window[\"ytInitialPlayerResponse\"]" {
		yy_push_state(YTINITIALPLAYERRESPONSE, yyscanner);
	}
}

<YTINITIALPLAYERRESPONSE>{
	"\"reason\":{\"simpleText\":\"Private video\"}" {
		for (int i = 2; i < 26; i++)
			sqlite3_bind_null(video_stmt, i);

		int status;
		if ((status = sqlite3_step(video_stmt)) != SQLITE_DONE)
			PANIC("sqlite3_step returned %d", status);

		SSL_shutdown(ssl);
		close(server);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		return 1;
	}
}

<PRIMARY_INFO>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		memcpy(title, yytext+26, yyleng-25);
		sqlite3_bind_text(video_stmt, 2, title, yyleng-25, SQLITE_STATIC);
		// For some reason the following line will not work
		// sqlite3_bind_text(video_stmt, 2, yytext+26, yyleng-25, SQLITE_STATIC);
	}
	"\"viewCount\":{\"simpleText\":\""[0-9,]+ {
		sqlite3_bind_int64(video_stmt, 3, stringToInt64(yytext+27));
	}
	"\"tooltip\":\""[0-9,]+" / "[0-9,]+ {
		ratings_found = 1;
		yytext += 11;
		sqlite3_bind_int64(video_stmt, 4, stringToInt64(yytext));
		yytext = strstr(yytext, "/");
		sqlite3_bind_int64(video_stmt, 5, stringToInt64(yytext+2));
	}
	"\"videoSecondaryInfoRenderer\"" {
		if (!ratings_found) {
			sqlite3_bind_null(video_stmt, 4);
			sqlite3_bind_null(video_stmt, 5);
		}
		yy_pop_state(yyscanner);
		yy_push_state(SECONDARY_RENDERER, yyscanner);
	}
}

<SECONDARY_RENDERER>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		memcpy(channel_name, yytext+26, yyleng-25);
		channel_name[yyleng-25] = '\0';
	}
	"\"url\":\"/channel/UC"[0-9a-zA-Z_-]{22} {
		int64_t l_id = decode64(yytext+18);
		int64_t r_id = decode64(yytext+29);
		sqlite3_bind_int64(video_stmt, 6, l_id);
		sqlite3_bind_int64(video_stmt, 7, r_id);
		if (channel_insert(l_id, r_id)) {
			if (sqlite3_prepare_v2(db, sql_channel_insert, -1, &channel_stmt, NULL) != SQLITE_OK) {
				sqlite3_close(db);
				PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));
			}
			sqlite3_bind_int64(channel_stmt, 1, l_id);
			sqlite3_bind_int64(channel_stmt, 2, r_id);
			sqlite3_bind_text(channel_stmt, 3, channel_name, strlen(channel_name), SQLITE_STATIC);
			yy_push_state(SUB_COUNT, yyscanner);
		}
	}
	"\"metadataRowContainer\"" {
		yy_push_state(METADATA, yyscanner);
	}
	"\"secondaryResults\"" {
		yy_pop_state(yyscanner);
		yy_push_state(SECONDARY_RESULTS, yyscanner);
	}
}

<SUB_COUNT>"\"subscriberCountText\":{\"runs\":[{\"text\":\""[^ ]+ {
	sqlite3_bind_int64(channel_stmt, 4, approximateSubs(yytext+40));
	int status;
	if ((status = sqlite3_step(channel_stmt)) != SQLITE_DONE)
		PANIC("sqlite3_step returned %d", status);
	sqlite3_finalize(channel_stmt);
	yy_pop_state(yyscanner);
}

<METADATA>{
	"\"text\":\"Age-restricted video" {
		for (int i = 8; i < 26; i++)
			sqlite3_bind_null(video_stmt, i);

		int status;
		if ((status = sqlite3_step(video_stmt)) != SQLITE_DONE)
			PANIC("sqlite3_step returned %d", status);

		SSL_shutdown(ssl);
		close(server);
		SSL_free(ssl);
		SSL_CTX_free(ctx);

		return 1;
	}

	"\"collapsedItemCount\"" {
		yy_pop_state(yyscanner);
	}
}

<SECONDARY_RESULTS>{
	"\"compactVideoRenderer\"" {
		yy_push_state(COMPACT_VIDEO_RENDERER, yyscanner);
	}
}

<COMPACT_VIDEO_RENDERER>{
	"\"videoId\":\""[0-9a-zA-Z_-]{11} {
		int64_t rec = decode64(yytext+11);
		sqlite3_bind_int64(video_stmt, rec_count+8, rec);
		if (video_insert(rec)) {
			push(rec);
		}

		yy_pop_state(yyscanner);
		if (++rec_count == 18) {
			int status;
			if ((status = sqlite3_step(video_stmt)) != SQLITE_DONE)
				PANIC("sqlite3_step returned %d", status);
			sqlite3_finalize(video_stmt);

			SSL_shutdown(ssl);
			close(server);
			SSL_free(ssl);
			SSL_CTX_free(ctx);
			if ((v_table_count - Q_Count) % 100 == 0)
				printf("processed: %lu, waiting: %lu, total: %lu, channels: %lu\n",
				v_table_count-Q_Count, Q_Count, v_table_count, c_table_count);
			return 1;
		}
	}
}

"\"currentVideoEndpoint\""|"</body>" {
	char id_str[11];

/*	debugging */
//	if (yytext[0] == '<')
//		fprintf(stderr, "currentVideoEndpoint not found: %.11s\n", encode64(id, id_str));
//	if (rec_count == 0)
//		fprintf(stderr, "rec_count = 0: %.11s\n", encode64(id, id_str));

	SSL_shutdown(ssl);
	close(server);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	push(id);
	return 1;
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
