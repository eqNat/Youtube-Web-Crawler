#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sqlite3.h>
#include <unistd.h>

#include "crawler.h"
#include "conversions.h"
#include "panic.h"
#include "dbcache/queue.h"
#include "dbcache/hash_table.h"

// tail-recursive function.
// Be careful when turning down optimizations for debugging.
__attribute__ ((noreturn))
void *logger(void *no_args)
{
    static uint64_t seconds = 0;

    printf("%lu: processed: %lu, waiting: %lu, total: %lu, channels: %lu\n",
        seconds++, v_table_count-Q_Count, Q_Count, v_table_count, c_table_count);
    sleep(1);

    logger(NULL);
}

int main(int argc, char *argv[])
{
    {// create database if not exists, else load database into cache (queue and hashmap)
        sqlite3 *db;
        char *zErrMsg = 0;

        if (sqlite3_open("youtube.db", &db) != SQLITE_OK)
            PANIC("Can't open database: %s", sqlite3_errmsg(db));

        char *create_video_table =
            "CREATE TABLE IF NOT EXISTS videos ("
            "id INTEGER(64) PRIMARY KEY,"
            "title VARCHAR(100)," // NULL if video is private
            "views INTEGER(64)," // NULL if private
            "likes INTEGER(64),"    // NULL if ratings are disabled OR private
            "dislikes INTEGER(64)," // 
            "lchannel_id INTEGER(64)," // NULL if video is private
            "rchannel_id INTEGER(64)," //
            "rec_1 INTEGER(64),"  // NULL if video is age-restricted OR private
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

        const char sql_id_select[] = "SELECT id FROM videos";

        sqlite3_stmt *res;

        if (sqlite3_prepare_v2(db, sql_id_select, -1, &res, 0) != SQLITE_OK)
            PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));

        int32_t status;
        // load video IDs into cache
        while ((status = sqlite3_step(res)) == SQLITE_ROW)
            video_insert(sqlite3_column_int64(res, 0));

        if (status != SQLITE_DONE)
            PANIC("step failed");

        sqlite3_finalize(res);

        const char sql_cid_select[] = "SELECT l_id, r_id FROM channels";

        if (sqlite3_prepare_v2(db, sql_cid_select, -1, &res, 0) != SQLITE_OK)
            PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));

        // load channel IDs into cache
        while ((status = sqlite3_step(res)) == SQLITE_ROW) {
            int64_t l_id = sqlite3_column_int64(res, 0);
            int64_t r_id = sqlite3_column_int64(res, 1);
            channel_insert(l_id, r_id);
        }

        if (status != SQLITE_DONE)
            PANIC("step failed");

        sqlite3_finalize(res);

        const char sql_rec_select[] =
            "SELECT "
            "rec_1, rec_2, rec_3, rec_4, rec_5, rec_6, rec_7, rec_8, rec_9, "
            "rec_10, rec_11, rec_12, rec_13, rec_14, rec_15, rec_16 "
            "FROM videos;";

        if (sqlite3_prepare_v2(db, sql_rec_select, -1, &res, 0) != SQLITE_OK)
            PANIC("Failed to prepare statement: %s", sqlite3_errmsg(db));

        while ((status = sqlite3_step(res)) == SQLITE_ROW)
            for (int32_t i = 0; i < 18; i++) {
                int64_t recommendation = sqlite3_column_int64(res, i);
                if (video_insert(recommendation))
                    enqueue(recommendation);
            }
        sqlite3_close(db);
    }

    {// insert seed ID if cache is empty
        int64_t start_id = decode64("hsWr_JWTZss");
        if (video_insert(start_id))
            enqueue(start_id);
    }

    printf("starting the threads\n");

    {// multithreading setup and execution
        uint8_t thread_num = 4;
        // set number of threads from argv if exists
        if (argc > 1)
            thread_num = (uint8_t) stringToInt64(argv[1]);
            pthread_t tids[thread_num+1];

        // set up logger
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tids[0], &attr, logger, NULL);

        // set up crawlers
        for (int32_t i = 1; i < thread_num+1; i++) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);       /***********************/
            pthread_create(&tids[i], &attr, /**/ crawler_wrapper /**/, NULL);
            printf("thread %d in\n", i);    /***********************/
            while (Q_Count < 5)
                sleep(1);
        }

        // We don't care about joining with logger
        for (int32_t i = 1; i < thread_num+1; i++)
            pthread_join(tids[i], NULL);
    }
}
