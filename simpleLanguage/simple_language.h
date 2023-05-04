#ifndef SIMPLE_LANGUAGE_H
#define SIMPLE_LANGUAGE_H

#include <stdio.h>
#include <stdlib.h>

#ifndef smalloc
#define smalloc malloc
#endif

#ifndef sfree
#define sfree	free
#endif

#ifndef sprint
#define sprint(str) printf("%s", str)
#endif

#ifndef SIMPLE_LANGUAGE_MAX_WORD_LEN
#define SIMPLE_LANGUAGE_MAX_WORD_LEN 100
#endif

#ifndef SIMPLE_LANGUAGE_ERROR_LEN 
#define SIMPLE_LANGUAGE_ERROR_LEN 30
#endif

/**
	@param code has to be freed manually
*/
void simpleLangExecute(const char* code, const unsigned short code_size);

#endif