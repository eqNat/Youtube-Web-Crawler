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
#define YY_INPUT(buf, result, max_size)	result = SSL_read(yyextra->ssl, buf, max_size)

/* debugging */
// #define YY_USER_ACTION fprintf(stderr, "%s", yytext);

struct flex_io {
	SSL *ssl;
	sqlite3 *db;
	sqlite3_stmt *video_stmt;
};

%}

%option noyywrap stack reentrant
%option extra-type="struct flex_io *"

%s YTINITIALDATA
%s YTINITIALPLAYERRESPONSE
%s PRIMARY_INFO
%s SECONDARY_RENDERER
%s SUB_COUNT
%s SECONDARY_RESULTS
%s METADATA
%s COMPACT_VIDEO_RENDERER

%%
 int rec_count = 0;
 _Bool ratings_found = 0;
 sqlite3_stmt *channel_stmt;
 char channel_name[256];

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
			sqlite3_bind_null(yyextra->video_stmt, i);

		int status;
		if ((status = sqlite3_step(yyextra->video_stmt)) != SQLITE_DONE)
			PANIC("sqlite3_step returned %d", status);
		return 1;
	}
}

<PRIMARY_INFO>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		sqlite3_bind_text(yyextra->video_stmt, 2, yytext+26, yyleng-25, SQLITE_TRANSIENT);
	}
	"\"viewCount\":{\"simpleText\":\""[0-9,]+ {
		sqlite3_bind_int64(yyextra->video_stmt, 3, stringToInt64(yytext+27));
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
		sqlite3_bind_int64(yyextra->video_stmt, 6, l_id);
		sqlite3_bind_int64(yyextra->video_stmt, 7, r_id);
		if (channel_insert(l_id, r_id)) {
			static const char sql_channel_insert[] = "INSERT INTO channels VALUES(?,?,?,?);";
			if (sqlite3_prepare_v2(yyextra->db, sql_channel_insert, -1, &channel_stmt, NULL) != SQLITE_OK) {
				sqlite3_close(yyextra->db);
				PANIC("Failed to prepare statement: %s", sqlite3_errmsg(yyextra->db));
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
			sqlite3_bind_null(yyextra->video_stmt, i);

		int status;
		if ((status = sqlite3_step(yyextra->video_stmt)) != SQLITE_DONE)
			PANIC("sqlite3_step returned %d", status);
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
		sqlite3_bind_int64(yyextra->video_stmt, rec_count+8, rec);
		if (video_insert(rec))
			push(rec);

		yy_pop_state(yyscanner);
		if (++rec_count == 18) {
			int status;
			if ((status = sqlite3_step(yyextra->video_stmt)) != SQLITE_DONE)
				PANIC("sqlite3_step returned %d", status);
			sqlite3_finalize(yyextra->video_stmt);

			if ((v_table_count - Q_Count) % 100 == 0)
				printf("processed: %lu, waiting: %lu, total: %lu, channels: %lu\n",
				v_table_count-Q_Count, Q_Count, v_table_count, c_table_count);
			return 1;
		}
	}
}

"\"currentVideoEndpoint\""|"</body>" {
/*	debugging */
//	char id_str[11];
//	if (yytext[0] == '<')
//		fprintf(stderr, "currentVideoEndpoint not found: %.11s\n", encode64(id, id_str));
//	if (rec_count == 0)
//		fprintf(stderr, "rec_count = 0: %.11s\n", encode64(id, id_str));

	return 0;
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
