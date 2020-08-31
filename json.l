%{
#include <string.h>
#include <stdint.h>

#include <sqlite3.h>

#include <openssl/ssl.h>

#include "json.h"
#include "conversions.h"
#include "panic.h"
#include "dbcache/queue.h"
#include "dbcache/hash_table.h"

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) result = SSL_read(yyextra->ssl, buf, max_size)

/* debugging */
//#define YY_USER_ACTION fprintf(stderr, "%s", yytext);

struct flex_io {
    SSL *ssl;
    sqlite3 *db;
    sqlite3_stmt *video_stmt;
    sqlite3_stmt *channel_stmt;
};

pthread_mutex_t lock;

%}

%option noyywrap reentrant
%option extra-type="struct flex_io *"

%s HTTP_HEADER
%s YTINITIALDATA
%s YTINITIALPLAYERRESPONSE
%s PRIMARY_INFO
%s SECONDARY_RENDERER
%s SUB_COUNT
%s SECONDARY_RESULTS
%s METADATA
%s COMPACT_VIDEO_RENDERER

%%
 
 int64_t id;
 int rec_count = 0;
 _Bool ratings_found = 0;
 _Bool should_insert_channel = 0;
 char channel_name[256];
 BEGIN(HTTP_HEADER);

<HTTP_HEADER>{
    "HTTP/1.1 4"[0-9]{2}* {
        // display 4xx errors
        PANIC("%.*s", yyleng, yytext);
    }

    "\r\n\r\n" {
        BEGIN(INITIAL);
    }
}

"window[\"ytInitialData\"] = " {
    //start of JSON object 
    BEGIN(YTINITIALDATA);
}

<YTINITIALDATA>{
    "\"videoPrimaryInfoRenderer\"" {
        BEGIN(PRIMARY_INFO);
    }
    "window[\"ytInitialPlayerResponse\"]" {
        // ytInitialData is empty causing us to enter a new JSON object
        // We will search ytInitialPlayerResponse for a reason why ytInitialData is empty
        BEGIN(YTINITIALPLAYERRESPONSE);
    }
}

<YTINITIALPLAYERRESPONSE>{
    "\"reason\":{\"simpleText\":\"Private video\"}" {
        // user changed video from public to private
        for (int i = 2; i < 24; i++)
            sqlite3_bind_null(yyextra->video_stmt, i);

        rec_count = 16;
        BEGIN(INITIAL);
    }
}

<PRIMARY_INFO>{
    "\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
        sqlite3_bind_text(yyextra->video_stmt, 2, yytext+26, yyleng-25, SQLITE_TRANSIENT);
    }
    "\"viewCount\":{\"simpleText\":\""[0-9,]+ {
        sqlite3_bind_int64(yyextra->video_stmt, 3, stringToInt64(yytext+27));
    }
    "\"videoId\":\""[0-9a-zA-Z_-]{11} {
        id = decode64(yytext+11);
        sqlite3_bind_int64(yyextra->video_stmt, 1, id);
    }
    "\"tooltip\":\""[0-9,]+" / "[0-9,]+ {
        ratings_found = 1;
        yytext += 11;
        sqlite3_bind_int64(yyextra->video_stmt, 4, stringToInt64(yytext));
        yytext = strstr(yytext, "/");
        sqlite3_bind_int64(yyextra->video_stmt, 5, stringToInt64(yytext+2));
    }
    "\"videoSecondaryInfoRenderer\"" {
        if (!ratings_found) {
            sqlite3_bind_null(yyextra->video_stmt, 4);
            sqlite3_bind_null(yyextra->video_stmt, 5);
        }
        BEGIN(SECONDARY_RENDERER);
    }
}

<SECONDARY_RENDERER>{
    "\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
        // get title
        memcpy(channel_name, yytext+26, yyleng-25);
        channel_name[yyleng-25] = '\0';
    }
    "\"url\":\"/channel/UC"[0-9a-zA-Z_-]{22} {
        // get channel ID
        int64_t l_id = decode64(yytext+18);
        int64_t r_id = decode64(yytext+29);
        sqlite3_bind_int64(yyextra->video_stmt, 6, l_id);
        sqlite3_bind_int64(yyextra->video_stmt, 7, r_id);
        if (channel_insert(l_id, r_id)) {
            should_insert_channel = 1;
            sqlite3_bind_int64(yyextra->channel_stmt, 1, l_id);
            sqlite3_bind_int64(yyextra->channel_stmt, 2, r_id);
            sqlite3_bind_text(yyextra->channel_stmt, 3, channel_name, strlen(channel_name), SQLITE_STATIC);
            BEGIN(SUB_COUNT);
        }
    }
    "\"metadataRowContainer\"" {
        BEGIN(METADATA);
    }
    "\"secondaryResults\"" {
        BEGIN(SECONDARY_RESULTS);
    }
}

<SUB_COUNT>{
    "\"subscriberCountText\":{\"runs\":[{\"text\":\""[^ ]+ {
        sqlite3_bind_int64(yyextra->channel_stmt, 4, approximateSubs(yytext+40));
        BEGIN(SECONDARY_RENDERER);
    }
    "\"metadataRowContainer\"" {
        // subscriber count is hidden
        sqlite3_bind_null(yyextra->channel_stmt, 4);
        BEGIN(METADATA);
    }
}

<METADATA>{
    "\"text\":\"Age-restricted video" {
        for (int i = 8; i < 24; i++)
            sqlite3_bind_null(yyextra->video_stmt, i);

        rec_count = 16;
        BEGIN(INITIAL);
    }

    "\"collapsedItemCount\"" {
        BEGIN(SECONDARY_RENDERER);
    }
}

<SECONDARY_RESULTS>{
    "\"compactVideoRenderer\"" {
        BEGIN(COMPACT_VIDEO_RENDERER);
    }
}

<COMPACT_VIDEO_RENDERER>{
    "\"label\":\"LIVE NOW\"," {
        // skip livestreams
        BEGIN(SECONDARY_RESULTS);
    }

    "\"text\":\"YouTube Movies\"," {
        // skip YouTube Movies
        BEGIN(SECONDARY_RESULTS);
    }

    "\"addedVideoId\":\""[0-9a-zA-Z_-]{11} {
        int64_t rec = decode64(yytext+16);
        sqlite3_bind_int64(yyextra->video_stmt, rec_count+8, rec);
        if (video_insert(rec))
            enqueue(rec);

        if (++rec_count == 16)
            BEGIN(INITIAL);
        else
            BEGIN(SECONDARY_RESULTS);
    }
}

"</html>" {
    BEGIN(HTTP_HEADER);

    if (rec_count != 16) {
        enqueue(id);
        return 0;
    }

    pthread_mutex_lock(&lock);
    sqlite3_exec(yyextra->db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    int status = sqlite3_step(yyextra->video_stmt);
    if (status != SQLITE_DONE)
        PANIC("sqlite3_step returned %d", status);

    if (should_insert_channel) {
        status = sqlite3_step(yyextra->channel_stmt);
        if (status != SQLITE_DONE)
            PANIC("sqlite3_step returned %d", status);
    }

    sqlite3_reset(yyextra->channel_stmt);
    sqlite3_reset(yyextra->video_stmt);

    sqlite3_exec(yyextra->db, "COMMIT;", NULL, NULL, NULL);
    pthread_mutex_unlock(&lock);

    return 1;
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
