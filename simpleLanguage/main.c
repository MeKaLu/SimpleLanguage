#include <stdio.h>
#include <stdlib.h>

#include "simple_language.h"

int main() {
	FILE * pfile;
	long lsize;
	char * buffer;
	size_t result;

	pfile = fopen ("example.simple", "rb");
	if (pfile == NULL) { fputs ("File error", stderr); exit (1); }

	// obtain file size:
	fseek (pfile, 0, SEEK_END);
	lsize = ftell(pfile);
	rewind(pfile);

	// allocate memory to contain the whole file:
	buffer = (char*)malloc(sizeof(char) * lsize);
	if (buffer == NULL) { fputs("Memory error", stderr); exit(2); }

	// copy the file into the buffer:
	result = fread(buffer, 1 ,lsize, pfile);
	if (result != lsize) { fputs ("Reading error", stderr); exit (3); }

	fclose(pfile);

	/* the whole file is now loaded in the memory buffer. */

	simpleLangExecute(buffer, lsize);

	free (buffer);

	getch();
	return 0;		
}
