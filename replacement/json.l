%{
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "json.h"
#include "conversions.h"

extern void yyerror(const char *message);
int rec_count = 0;

struct Channel_Row cRow;

struct Video_Row vRow = {
	.likes = -1,
	.dislikes = -1,
	.channel_fk = &(cRow.id)
};

%}

%option noyywrap
%option stack

%s WINDOW
%s PRIMARY_INFO
%s SECOND_RENDERER
%s SECOND_RESULTS
%s COMPACT_VIDEO_RENDERER

%%

"window[\"ytInitialData\"] = " {
	yy_push_state(WINDOW);
}

<WINDOW>"\"videoPrimaryInfoRenderer\"" {
	yy_push_state(PRIMARY_INFO);
}

<PRIMARY_INFO>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		memcpy(vRow.title, yytext+26, yyleng-25);
	}
	"\"viewCount\":{\"simpleText\":\""[0-9,]+ {
		vRow.views = stringToInt64(yytext+27);
	}
	"\"tooltip\":\""[0-9,]+" / "[0-9,]+ {
		yytext += 11;
		vRow.likes = stringToInt64(yytext);
		yytext = strstr(yytext, "/");
		vRow.dislikes = stringToInt64(yytext+2);
	}
	"\"videoSecondaryInfoRenderer\"" {
		yy_pop_state();
		yy_push_state(SECOND_RENDERER);
	}
}

<SECOND_RENDERER>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		memcpy(cRow.name, yytext+26, yyleng-25);
	}
	"\"url\":\"/channel/UC"[0-9a-zA-Z_-]{22} {
		cRow.id = decode128(yytext+18);
	}
	"\"subscriberCountText\":{\"runs\":[{\"text\":\""[^ ]+ {
		cRow.subscribers = approximateSubs(yytext+40);
	}
	"\"secondaryResults\"" {
		yy_pop_state();
		yy_push_state(SECOND_RESULTS);
	}
}

<SECOND_RESULTS>{
	"\"compactVideoRenderer\"" {
		yy_push_state(COMPACT_VIDEO_RENDERER);
	}
}

<COMPACT_VIDEO_RENDERER>{
	"\"videoId\": \""[0-9a-zA-Z_-]{11} {
		vRow.recommendations[rec_count] = decode64(yytext+12);
		yy_pop_state();
		if (++rec_count == 18)
			return 1;
	}
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
