#include <stdio.h>
#include <sqlite3.h> 

#include "crawler.h"
#include "json.h"
#include "queue.h"
#include "conversions.h"
#include "panic.h"

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

__attribute__ ((constructor))
void ssl_init()
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
}

const char sql_video_insert[] = "INSERT INTO videos VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

void crawler(struct flex_io *io, yyscan_t scanner, SSL_CTX *ctx)
{
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
		struct sockaddr_in yt_address = {
			.sin_family = AF_INET,
			.sin_port = htons(443)
		};

		if (inet_pton(AF_INET, "172.217.1.238", &yt_address.sin_addr) != 1)
			PANIC("inet_pton failed");

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
