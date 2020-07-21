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
%s SECONDARY_RENDERER
%s SECONDARY_RESULTS
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
		printf("Title: %s\n", vRow.title);
	}
	"\"viewCount\":{\"simpleText\":\""[0-9,]+ {
		vRow.views = stringToInt64(yytext+27);
		printf("Views: %ld\n", vRow.views);
	}
	"\"tooltip\":\""[0-9,]+" / "[0-9,]+ {
		yytext += 11;
		vRow.likes = stringToInt64(yytext);
		yytext = strstr(yytext, "/");
		vRow.dislikes = stringToInt64(yytext+2);
		printf("Likes: %ld\n", vRow.likes);
		printf("Dislikes: %ld\n", vRow.dislikes);
	}
	"\"videoSecondaryInfoRenderer\"" {
		yy_pop_state();
		yy_push_state(SECONDARY_RENDERER);
	}
}

<SECONDARY_RENDERER>{
	"\"title\":{\"runs\":[{\"text\":"\"(\\.|[^"\\])*/\" {
		memcpy(cRow.name, yytext+26, yyleng-25);
		printf("Channel name: %s\n", cRow.name);
	}
	"\"url\":\"/channel/UC"[0-9a-zA-Z_-]{22} {
		cRow.id.l_half = decode64(yytext+18);
		cRow.id.r_half = decode64(yytext+29);
		printf("channel ID (base-64) %s, (base-16) %lx%lx\n", yytext+18, cRow.id.l_half, cRow.id.r_half);
	}
	"\"subscriberCountText\":{\"runs\":[{\"text\":\""[^ ]+ {
		cRow.subscribers = approximateSubs(yytext+40);
		printf("subscribers (Three significant figures): %ld\n", cRow.subscribers);
	}
	"\"secondaryResults\"" {
		yy_pop_state();
		yy_push_state(SECONDARY_RESULTS);
	}
}

<SECONDARY_RESULTS>{
	"\"compactVideoRenderer\"" {
		yy_push_state(COMPACT_VIDEO_RENDERER);
	}
}

<COMPACT_VIDEO_RENDERER>{
	"\"videoId\": \""[0-9a-zA-Z_-]{11} {
		vRow.recommendations[rec_count] = decode64(yytext+12);
		printf("rec %d :(base-64)%s, (base-16)%lx\n", rec_count+1, yytext+12, vRow.recommendations[rec_count]);
		yy_pop_state();
		if (++rec_count == 18)
			return 1;
	}
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
