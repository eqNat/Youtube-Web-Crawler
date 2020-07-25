%{
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "json.h"
#include "conversions.h"
#include "panic.h"

#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) result = SSL_read(ssl, buf, max_size)

SSL *ssl;

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
 int server;
 SSL_CTX *ctx;
 {//initialize
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
		PANIC("SSL_CTX_new() failed.");

	// socket()
	server = socket(AF_INET6, SOCK_STREAM, 0);
	if (server < 0)
		PANIC("socket() failed. (%d)", errno);

	{// connect()
		struct sockaddr_in6 server_address = {0};
		server_address.sin6_family = AF_INET6;
		server_address.sin6_port = htons(443);

		if (inet_pton(AF_INET6, "2607:f8b0:4000:812::200e", &server_address.sin6_addr) != 1)
			PANIC("inet_pton failed");

		if (connect(server, (struct sockaddr*)&server_address, sizeof(server_address)) == -1)
			PANIC("connect() failed. (%d)\n", errno);
	}

	ssl = SSL_new(ctx);
	if (!ctx)
		PANIC("SSL_new() failed.");

	SSL_set_fd(ssl, server);
	if (SSL_connect(ssl) == -1) {
		ERR_print_errors_fp(stderr);
		PANIC("SSL_connect() failed.");
	}

	char request[] = "GET /watch?v=3nrLc_JOF7k HTTP/1.1\r\n"
	                 "Host: www.youtube.com:443\r\n"
	                 "Connection: close\r\n"
	                 "User-Agent: https_simple\r\n\r\n";

	SSL_write(ssl, request, sizeof(request)-1);
 }

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
	"\"videoId\":\""[0-9a-zA-Z_-]{11} {
		vRow.recommendations[rec_count] = decode64(yytext+11);
		printf("rec %d :(base-64)%s, (base-16)%lx\n", rec_count+1, yytext+12, vRow.recommendations[rec_count]);
		yy_pop_state();
		if (++rec_count == 18) {
			SSL_shutdown(ssl);
			close(server);
			SSL_free(ssl);
			SSL_CTX_free(ctx);
			return 1;
		}
	}
}

[ \t\n]+          /* ignore whitespace */
. {}

%%
