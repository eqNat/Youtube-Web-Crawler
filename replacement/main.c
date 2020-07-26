#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "json.h"
#include "conversions.h"
#include "queue.h"
#include "hash_table.h"

int main(int argc, char *argv[])
{
	//char start_id[] = "3nrLc_JOF7k"; // ordinary
	char start_id[] = "x71MDrC400A"; // age-restricted
	int64_t start_id_int = decode64(start_id);

	push(start_id_int);
	video_insert(start_id_int);

	yylex();
}
