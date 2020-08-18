# Youtube-Web-Crawler

Dependencies: sqlite3 and openssl

To compile, type

	make

To run, type
	
	./crawler [thread_num]

Without specifying an argument, the number of threads defaults to four.

This crawler will create a sqlite3 database file named 'youtube.db' if one doesn't aleady exist. The database consists of two tables: 'videos' and 'channels'. The tables are created using the following statements:


	CREATE TABLE IF NOT EXISTS videos (
	id INTEGER(64) PRIMARY KEY,
	title VARCHAR(100), -- NULL if video is private
	views INTEGER(64), -- NULL if title='Youtube Movies' OR private
	likes INTEGER(64),    -- NULL if ratings are disabled OR private
	dislikes INTEGER(64), -- 
	lchannel_id INTEGER(64), -- NULL if video is private
	rchannel_id INTEGER(64), --
	rec_1 INTEGER(64),  -- NULL if video is age-restricted OR private
	rec_2 INTEGER(64),  --
	rec_3 INTEGER(64),  --
	rec_4 INTEGER(64),  --
	rec_5 INTEGER(64),  --
	rec_6 INTEGER(64),  --
	rec_7 INTEGER(64),  --
	rec_8 INTEGER(64),  --
	rec_9 INTEGER(64),  --
	rec_10 INTEGER(64), --
	rec_11 INTEGER(64), --
	rec_12 INTEGER(64), --
	rec_13 INTEGER(64), --
	rec_14 INTEGER(64), --
	rec_15 INTEGER(64), --
	rec_16 INTEGER(64), --
	FOREIGN KEY(lchannel_id, rchannel_id)  -- NULL if video is private
		REFERENCES channels(l_id, r_id)); --


	CREATE TABLE IF NOT EXISTS channels (
	l_id INTEGER(64),
	r_id INTEGER(64),
	name VARCHAR(20) NOT NULL,
	subscribers INTEGER(64), -- NULL if subscriber count is hidden
	PRIMARY KEY (l_id, r_id));
