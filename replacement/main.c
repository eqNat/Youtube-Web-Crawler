#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "json.h"

extern struct Video_Row vRow;
extern struct Channel_Row cRow;

int main(int argc, char *argv[])
{
	yyin = (argc == 2) ? fopen(argv[1],"r") : fopen("yt.json", "r");

	yylex();
}
