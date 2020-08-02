#include <stdio.h>
#include <sqlite3.h> 

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/ssl.h>

#include "crawler.h"
#include "json.h"
#include "queue.h"
#include "conversions.h"
#include "panic.h"

const struct sockaddr_in yt_address;

__attribute__ ((constructor))
void yt_address_init()
{
	struct sockaddr_in *p_address = &yt_address;

	p_address->sin_family = AF_INET;
	p_address->sin_port = htons(443);

	if (inet_pton(AF_INET, "172.217.1.238", &p_address->sin_addr) != 1)
		PANIC("inet_pton failed");
}

const char sql_video_insert[] = "INSERT INTO videos VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

struct flex_io { // yyextra
	SSL *ssl;
	sqlite3 *db;
	sqlite3_stmt *video_stmt;
};

void crawler(struct flex_io *io, yyscan_t scanner, SSL_CTX *ctx)
{
	yt_address_init();
	int64_t id = pop();
	if (!id)
		return;

	if (sqlite3_prepare_v2(io->db, sql_video_insert, -1, &(io->video_stmt), NULL) != SQLITE_OK) {
		sqlite3_close(io->db);
		PANIC("Failed to prepare statement: %s", sqlite3_errmsg(io->db));
	}
	sqlite3_bind_int64(io->video_stmt, 1, id);

	int client = socket(AF_INET, SOCK_STREAM, 0);
	if (client < 0)
		PANIC("socket() failed. (%d)", errno);

	{// securely connect to Youtube
		for (int i = 0; connect(client, (struct sockaddr*)&yt_address, sizeof(yt_address)) == -1; i++)
			if (i == 5)
				PANIC("connect() failed. (%d)\n", errno);

		io->ssl = SSL_new(ctx);
		if (!ctx)
			PANIC("SSL_new() failed.");

		SSL_set_fd(io->ssl, client);
		for (int i = 0; SSL_connect(io->ssl) == -1; i++)
			if (i == 5) {
				close(client);
				PANIC("SSL_connect() failed. (%d)\n", errno);
			}
	}

	{// send request
		static _Thread_local char request[] =
			"GET /watch?v=########### HTTP/1.1\r\n"
			"Host: www.youtube.com:443\r\n"
			"Connection: close\r\n"
			"User-Agent: https_simple\r\n\r\n";

		encode64(id, request+13);
		SSL_write(io->ssl, request, sizeof(request)-1);
	}

	if (!yylex(scanner))
		push(id);

	SSL_shutdown(io->ssl);
	close(client);
	SSL_free(io->ssl);

	crawler(io, scanner, ctx); // tail-recursive call
}

void* crawler_wrapper(void* no_args)
{
	struct flex_io io; // yyextra
	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
		PANIC("SSL_CTX_new() failed.");

	{// Open database, 
		if (sqlite3_open("youtube.db", &(io.db)) != SQLITE_OK) {
			sqlite3_close(io.db);
			PANIC("Cannot open database: %s", sqlite3_errmsg(io.db));
		}
		int status;
		if ((status = sqlite3_exec(io.db, "PRAGMA synchronous = OFF", NULL, NULL, NULL)) != SQLITE_OK)
			PANIC("PRAGMA failed: sqlite3_exec returned %d", status);
		
		sqlite3_busy_timeout(io.db, 100);
	}

	yyscan_t scanner;

	yylex_init_extra(&io, &scanner);

	crawler(&io, scanner, ctx);

	printf("queue empty\n");

	sqlite3_close(io.db);
	SSL_CTX_free(ctx);

	yylex_destroy(scanner);

	return NULL;
}
