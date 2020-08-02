#pragma once

typedef void* yyscan_t;
struct flex_io;

int yylex(yyscan_t);
int yylex_destroy(yyscan_t);
int yylex_init_extra(struct flex_io *, yyscan_t *);
