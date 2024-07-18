#include "Utils.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

#pragma warning(disable : 4996)		//VS doesn't even like fopen()?! My God....


double frand()
{
	return (double)rand() / RAND_MAX;
}

double fsrand()
{
	return 2.0 * frand() - 1.0;
}

double current_time()
{
	return (double)clock() / CLOCKS_PER_SEC;
}

char* read_file(const char* filename)
{
	FILE *fin = fopen(filename, "r");
	fseek(fin, 0, SEEK_END);
	size_t length = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	char* ret = (char*)malloc(length + 1);
	if(ret)
	{
		fread(ret, length, 1, fin);
		ret[length] = 0;
	}
	fclose(fin);
	return ret;
}


void error(const char* fmt, ...)
{
	va_list arg_list;
	char buffer[4096];

	va_start(arg_list, fmt);
	vsprintf(buffer, fmt, arg_list);
	fprintf(stderr, buffer);
	va_end(arg_list);
	exit(-1);
}