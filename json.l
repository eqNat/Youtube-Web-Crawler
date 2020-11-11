%{
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <sqlite3.h>
#include <openssl/ssl.h>

#include "json.h"
#include "conversions.h"
#include "panic.h"
#include "dbcache/queue.h"
#include "dbcache/hash_table.h"
#include "chunker.h"

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) result = chunk_read(&yyextra->chunker, buf, max_size); \
                                                 write_history(buf, result)

/* debugging */
//#define YY_USER_ACTION fprintf(stderr, "%s", yytext);

struct flex_io {
    struct Chunker chunker;
    sqlite3 *db;
    sqlite3_stmt *video_stmt;
    sqlite3_stmt *channel_stmt;
    int64_t id;
};

_Thread_local int64_t tl_id;

void log_id()
{
    char id_str[11];
    fprintf(stderr ,"\nID = %.*s\n", 11, encode64(tl_id, id_str));
}

pthread_mutex_t lock;

%}

%option noyywrap reentrant
%option extra-type="struct flex_io *"

%s YTINITIALDATA
%s YTINITIALPLAYERRESPONSE
%s PRIMARY_INFO
%s SECONDARY_RENDERER
%s SUB_COUNT
%s SECONDARY_RESULTS
%s METADATA

%%
 
 tl_id = yyextra->id;
 int64_t id;
 int64_t rec_IDs[16];
 int rec_count = 0;
 _Bool ratings_found = 0;
 _Bool should_insert_channel = 0;
 char channel_name[256];
 BEGIN(YTINITIALDATA);

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

        rec_count = -1;
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

        rec_count = -1;
        BEGIN(INITIAL);
    }

    "\"collapsedItemCount\"" {
        BEGIN(SECONDARY_RENDERER);
    }
}

<SECONDARY_RESULTS>{
    "\"compactVideoRenderer\":{\"videoId\":\""[0-9a-zA-Z_-]{11} {
        if (rec_count == 16)
            BEGIN(INITIAL);
        else
            rec_IDs[rec_count++] = decode64(yytext + 35);
    }

    "\"label\":\"LIVE NOW\"," {
        // skip livestreams
        rec_count--;
        BEGIN(SECONDARY_RESULTS);
    }

    "\"text\":\"YouTube Movies\"," {
        // skip YouTube Movies
        rec_count--;
        BEGIN(SECONDARY_RESULTS);
    }

}

"</html>" {
    BEGIN(INITIAL);

    // (rec_count == -1) implies video is either private or age-restricted,
    // so it's expected to have no recommendations
    // (rec_count < 16 && rec_cound != -1) implies a problem
    if (rec_count != 16 && rec_count != -1) {
#ifdef LOG_BAD
        if (rec_count == 0) {
            pthread_mutex_lock(&lock);
            int fd = open("bad_IDs", O_APPEND | O_CREAT | O_WRONLY, 0666);
            write(fd, &id, sizeof(id));
            close(fd);
            pthread_mutex_unlock(&lock);
        }
#endif
        enqueue(id);
        sqlite3_reset(yyextra->channel_stmt);
        sqlite3_reset(yyextra->video_stmt);
        return 0;
    }

    for (int i = 0; i < rec_count; i++) {
        sqlite3_bind_int64(yyextra->video_stmt, i+8, rec_IDs[i]);
        if (video_insert(rec_IDs[i]))
            enqueue(rec_IDs[i]);
    }

    pthread_mutex_lock(&lock);
    sqlite3_exec(yyextra->db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    int status = sqlite3_step(yyextra->video_stmt);
    if (status != SQLITE_DONE) {
        char id_text[11];
        encode64(id, id_text);
        PANIC("sqlite3_step returned %d for id %s", status, id_text);
    }

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
