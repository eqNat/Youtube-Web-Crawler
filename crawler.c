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
#include "conversions.h"
#include "panic.h"
#include "dbcache/queue.h"

// max piped http requests per thread
// value 1 is most stable
// value 2 gives better performance but may crash program in more cases
// any higher than 2 will result in no performance gain and will be very unstable
#define PIPE_MAX 1 

inline void send_request(int64_t id);

// tail-recursive function.
// Be careful when turning down optimizations for debugging.
void crawler(yyscan_t scanner)
{
    static _Thread_local int64_t pipe_count = 0;

    do {
        int64_t id = dequeue();
        if (!id) {
            if (pipe_count == 0)
                return; // queue is empty and no response is headed our way
            break;
        }
        send_request(id);
    } while (++pipe_count < PIPE_MAX);

    /***********************/
    /**/ yylex(scanner); /**/
    /***********************/

    pipe_count--;
    crawler(scanner);
}

struct sockaddr_in yt_address;

__attribute__ ((constructor))
void yt_address_init()
{
    yt_address.sin_family = AF_INET;
    yt_address.sin_port = htons(443);

    if (inet_pton(AF_INET, "172.217.1.238", &yt_address.sin_addr) != 1)
        PANIC("inet_pton failed");
}

struct flex_io { // yyextra
    SSL *ssl;
    sqlite3 *db;
    sqlite3_stmt *video_stmt;
    sqlite3_stmt *channel_stmt;
} _Thread_local io;

void* crawler_wrapper(void* no_args)
{
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx)
        PANIC("SSL_CTX_new() failed.");

    {// Open database
        if (sqlite3_open("youtube.db", &(io.db)) != SQLITE_OK) 
            PANIC("Cannot open database: %s", sqlite3_errmsg(io.db));

        int status = sqlite3_exec(io.db, "PRAGMA synchronous = OFF", NULL, NULL, NULL);
        if (status != SQLITE_OK)
            PANIC("PRAGMA failed: sqlite3_exec returned %d", status);
    }

    {// Prepare and bind sql statement
        // video insertion statement
        const char sql_video_insert[] =
            "INSERT INTO videos VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
        if (sqlite3_prepare_v2(io.db, sql_video_insert, -1, &(io.video_stmt), NULL) != SQLITE_OK)
            PANIC("Failed to prepare statement: %s", sqlite3_errmsg(io.db));

        // channel insertion statement
        const char sql_channel_insert[] = "INSERT INTO channels VALUES(?,?,?,?);";
        if (sqlite3_prepare_v2(io.db, sql_channel_insert, -1, &(io.channel_stmt), NULL) != SQLITE_OK) {
            sqlite3_close(io.db);
            PANIC("Failed to prepare statement: %s", sqlite3_errmsg(io.db));
        }
    }

    int client;
    {// securely connect to Youtube
        if ((client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            PANIC("socket() failed. (%d)", errno);

        if (connect(client, (struct sockaddr*)&yt_address, sizeof(yt_address)) < 0)
            PANIC("connect() failed. (%d)", errno);

        io.ssl = SSL_new(ctx);
        if (!ctx)
            PANIC("SSL_new() failed.");

        SSL_set_fd(io.ssl, client);
        if (SSL_connect(io.ssl) < 0)
            PANIC("SSL_connect() failed. (%d)", errno);
    }

    yyscan_t scanner;
    yylex_init_extra(&io, &scanner);

    /*************************/
    /**/ crawler(scanner); /**/
    /*************************/

    printf("Thread leaving due to empty queue\n");

    // cleanup
    SSL_shutdown(io.ssl);
    close(client);
    SSL_free(io.ssl);
    SSL_CTX_free(ctx);
    sqlite3_close(io.db);
    sqlite3_finalize(io.channel_stmt);
    sqlite3_finalize(io.video_stmt);
    yylex_destroy(scanner);

    return NULL;
}

void send_request(int64_t id)
{
    static _Thread_local char request[] =
        "GET /watch?v=########### HTTP/1.1\r\n" // only hash characters should change
        "Host: www.youtube.com:443\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: https_simple\r\n\r\n";

    encode64(id, request+13);
    SSL_write(io.ssl, request, sizeof(request)-1);
}
