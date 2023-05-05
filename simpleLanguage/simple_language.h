#ifndef SIMPLE_LANGUAGE_H
#define SIMPLE_LANGUAGE_H

#include <cstdlib>

#ifndef smalloc
#define smalloc			malloc
#endif

#ifndef sfree
#define sfree			free
#endif

#ifndef SIMPLE_LANGUAGE_MAX_WORD_LEN
#define SIMPLE_LANGUAGE_MAX_WORD_LEN		15
#endif

#if SIMPLE_LANGUAGE_ERROR_MESSAGE
#ifndef SIMPLE_LANGUAGE_ERROR_LEN 
#define SIMPLE_LANGUAGE_ERROR_LEN			50
#endif
#endif

/**
	@param code has to be freed manually
*/
void simpleLangExecute(const char* code, const unsigned short code_size);

#endif