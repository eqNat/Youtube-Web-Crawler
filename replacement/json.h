#pragma once

#include <string.h>

extern FILE* yyin;
extern int yylineno;

int yylex();

struct Channel_Row {
	__int128 id; // primary key
	char name[20];
	int64_t subscribers;
};

struct Video_Row {
	int64_t id; // primary key
	char title[100];
	int64_t views;
	int64_t likes;
	int64_t dislikes;
	__int128 *channel_fk; // foriegn key
	int64_t recommendations[18];
};
