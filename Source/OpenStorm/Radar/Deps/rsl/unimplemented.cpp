#include <stdio.h>


int rsl_pclose(FILE* fp)
{
	int rc;
	rc = fclose(fp);
	return rc;
}

FILE* uncompress_pipe(FILE* fp)
{
	fprintf(stderr, "GZIP compressed radar files are not supported\n");
	return nullptr;
}