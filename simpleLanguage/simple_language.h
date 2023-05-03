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

/**
	@param code has to be freed manually
*/
void simpleLangExecute(const char* code, const unsigned short code_size);

#endif