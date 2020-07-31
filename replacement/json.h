#pragma once

typedef void* yyscan_t;

int yylex_init(yyscan_t*);
int yylex(yyscan_t);
int yylex_destroy(yyscan_t);
