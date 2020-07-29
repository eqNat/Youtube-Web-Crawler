#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <sqlite3.h> 

#include "json.h"
#include "conversions.h"
#include "queue.h"
#include "hash_table.h"
#include "panic.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
   for(int i = 0; i < argc; i++)
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   printf("\n");
   return 0;
}

int main()
{
	{
		sqlite3 *db;
		char *zErrMsg = 0;

		if (sqlite3_open("youtube.db", &db) != SQLITE_OK)
			PANIC("Can't open database: %s", sqlite3_errmsg(db));

		char *create_video_table =
			"CREATE TABLE IF NOT EXISTS videos ("
			"id INTEGER(64) PRIMARY KEY,"
			"title VARCHAR(100)," // NULL if video is private
			"views INTEGER(64)," // NULL if livestream OR title = 'Youtube Movies' OR private
			"likes INTEGER(64),"    // NULL if ratings are disabled OR private
			"dislikes INTEGER(64)," // 
			"lchannel_id INTEGER(64)," // NULL if video is private
			"rchannel_id INTEGER(64)," //
			"rec_1 INTEGER(64),"  // NULL if video is age-restricted
			"rec_2 INTEGER(64),"  //
			"rec_3 INTEGER(64),"  //
			"rec_4 INTEGER(64),"  //
			"rec_5 INTEGER(64),"  //
			"rec_6 INTEGER(64),"  //
			"rec_7 INTEGER(64),"  //
			"rec_8 INTEGER(64),"  //
			"rec_9 INTEGER(64),"  //
			"rec_10 INTEGER(64)," //
			"rec_11 INTEGER(64)," //
			"rec_12 INTEGER(64)," //
			"rec_13 INTEGER(64)," //
			"rec_14 INTEGER(64)," //
			"rec_15 INTEGER(64)," //
			"rec_16 INTEGER(64)," //
			"rec_17 INTEGER(64)," //
			"rec_18 INTEGER(64)," //
			"FOREIGN KEY(lchannel_id, rchannel_id)"  // NULL if video is private
				"REFERENCES channels(l_id, r_id));"; //

		if (sqlite3_exec(db, create_video_table, NULL, 0, &zErrMsg) != SQLITE_OK )
			PANIC("SQL error: %s", zErrMsg);

		char *create_channel_table =
			"CREATE TABLE IF NOT EXISTS channels ("
			"l_id INTEGER(64),"
			"r_id INTEGER(64),"
			"name VARCHAR(20) NOT NULL,"
			"subscribers INTEGER(64)," // NULL if subscriber count is hidden
			"PRIMARY KEY (l_id, r_id));";

		if (sqlite3_exec(db, create_channel_table, NULL, 0, &zErrMsg) != SQLITE_OK )
			PANIC("SQL error: %s", zErrMsg);

		const char sql_id_select[] = "SELECT id, lchannel_id, rchannel_id from videos";

		sqlite3_stmt *res;

		if (sqlite3_prepare_v2(db, sql_id_select, -1, &res, 0) != SQLITE_OK)
			PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));

		int status;
		while ((status = sqlite3_step(res)) == SQLITE_ROW) {
			video_insert(sqlite3_column_int64(res, 0));
			int64_t l_id = sqlite3_column_int64(res, 1);
			int64_t r_id = sqlite3_column_int64(res, 2);
			channel_insert(l_id, r_id);
		}

		if (status != SQLITE_DONE)
			PANIC("step failed");

		sqlite3_finalize(res);

		const char sql_rec_select[] =
			"SELECT "
			"rec_1, rec_2, rec_3, rec_4, rec_5, rec_6, rec_7, rec_8, rec_9, "
			"rec_10, rec_11, rec_12, rec_13, rec_14, rec_15, rec_16, rec_17, rec_18 "
			"FROM videos;";

		if (sqlite3_prepare_v2(db, sql_rec_select, -1, &res, 0) != SQLITE_OK)
			PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));

		while ((status = sqlite3_step(res)) == SQLITE_ROW)
			for (int i = 0; i < 18; i++) {
				int64_t recommendation = sqlite3_column_int64(res, i);
				if (video_insert(recommendation))
					push(recommendation);
			}
		sqlite3_close(db);
	}

	char start_id[] = "3nrLc_JOF7k"; // ordinary
	//char start_id[] = "x71MDrC400A"; // age-restricted
	if (v_table_count == 0) {
		int64_t start_id_int = decode64(start_id);

		push(start_id_int);
		video_insert(start_id_int);
	}

	while (yylex())
	{/* Empty while loop */}
	
	printf("Leaving");
	
}
