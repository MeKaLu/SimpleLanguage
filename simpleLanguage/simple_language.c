#include "simple_language.h"

#include <stdbool.h>

/*
	P -> pin
	F -> func
	A -> params
	C -> combine	[C|n(P)|C|F|n(A)]
	S -> condition	[P|S|n(B)|F|n(A)] or [C|n(P)|S|n(B)|F|n(A)]
*/
#define STATE_ZERO						'0'
#define STATE_PIN						'P'
#define STATE_FUNCTION					'F'
#define STATE_ARGUMENT					'A'
#define STATE_COMBINE					'C'
#define STATE_CONDITION					'S'
#define STATE_CONDITION_ARGUMENT		'B'

#define SPECIAL_COMMENT		'#'
#define SPECIAL_END			';'
#define SPECIAL_COMBINE		'@'
#define SPECIAL_CONDITION	':'

#if SIMPLE_LANGUAGE_ERROR_MESSAGE
#define set_error(message)  strcpy(error, message, SIMPLE_LANGUAGE_ERROR_LEN)
#else
#define set_error(message)
#endif

struct SimpleLangObject {
	void* ptr;
	const void* func;
	
	unsigned char size;
	char type;
	short id;
};
typedef struct SimpleLangObject SimpleLangObject;

SimpleLangObject* object_list = NULL;
unsigned short object_list_index = 0;
unsigned short object_list_size = 0;

static void objectListInit() {
	object_list_index = 0;
	object_list_size = 10;
	object_list = (SimpleLangObject*)smalloc(sizeof(SimpleLangObject) * object_list_size);
	for (unsigned short i = 0; i < object_list_size; i++)
		object_list[i] = (SimpleLangObject){ NULL, NULL, 0 , 0, -1 };
}

static void objectListFree() {
	for (unsigned short i = 0; i < object_list_size; i++)
		if (object_list[i].ptr != NULL) sfree(object_list[i].ptr);
	sfree(object_list);
	object_list = NULL;
	object_list_index = 0;
	object_list_size = 0;
}

static void objectListResize(unsigned short newsize) {
	SimpleLangObject* oldlist = object_list;

	const unsigned int oldsize = object_list_size;
	object_list_size = newsize;
	
	object_list = (SimpleLangObject*)smalloc(sizeof(SimpleLangObject) * object_list_size);
	for (unsigned short i = 0; i < object_list_size; i++) {
		if (i > oldsize) {
			object_list[i] = (SimpleLangObject){ NULL, NULL, 0, 0, -1 };
		} else if (i < oldsize) object_list[i] = oldlist[i];
	}
	sfree(oldlist);
}

// returns the position of the obj
static short objectListFind(short id) {
	for (short i = 0; i < object_list_index; i++)
		if (object_list[i].id == id) return i;
	return -1;
}

static bool objectListAppend(SimpleLangObject obj) {
	if (object_list_index >= object_list_size)
		return false;

	object_list[object_list_index] = obj;
	object_list_index++;
	return true;
}

static unsigned short strlen(const char* str) {
	unsigned short l = 0;
	while (str[l] != '\0') l++;
	return l;
}

static bool strcpy(char* dest, const char* source, unsigned short size) {
	const unsigned short so_len = strlen(source);
	if (so_len > size) return false;
	for (unsigned short i = 0; i < size; i++) {
		if (i >= so_len) dest[i] = '\0';
		else dest[i] = source[i];
	}
	return true;
}

static bool strcmp(const char* src1, const char* src2) {
	const unsigned short s_len = strlen(src1);
	if (s_len != strlen(src2)) return false;

	for (unsigned short i = 0; i < s_len; i++) {
		if (src1[i] != src2[i]) return false;
	}
	return true;
}

static void strfill(char* str, unsigned short size, const char fill) {
	for (unsigned short i = 0; i < size; i++)
		str[i] = fill;
}

void simpleLangExecute(const char* code, const unsigned short code_size) {
	objectListInit();

	unsigned short i = 0;
	unsigned short linec = 1;
	char c = 0;
	unsigned short object_id = 0;

#if SIMPLE_LANGUAGE_ERROR_MESSAGE
	char error[SIMPLE_LANGUAGE_ERROR_LEN];
#endif
	set_error("Unkown");

	unsigned char word_len = 0;
	char word[SIMPLE_LANGUAGE_MAX_WORD_LEN];
	strfill(word, SIMPLE_LANGUAGE_MAX_WORD_LEN, '\0');
	
	char old_state = STATE_ZERO;
	char state = STATE_ZERO;

	bool skip = false;

	while (i < code_size) {
		c = code[i];
		if (c == '\n') {
			linec++;
			word[0] = '\0';
			word_len = 0;
			if (skip) skip = false;
			else if (!skip) {
				if (state != STATE_ZERO) {
					// there should be ';' at the end of each statement but there was not
					set_error("UnfinishedStatement");
					goto force_error;
				}
			}
		} else if (c == '\r' || c == '\t') {
		} else if (c == SPECIAL_COMMENT) {
			skip = true;
		} else if (!skip) {
			// lines cannot start with special characters
			if (c == SPECIAL_COMBINE || c == SPECIAL_CONDITION || c == SPECIAL_END) {
				set_error("LinesCannotStartWithSpecialChars");
				goto force_error;
			} else if (c == ' ' && word_len > 0) {
				// a word

				// +1 for null character at the end, strcpy will add it
				word_len++;
				void* word_copy = smalloc(sizeof(char) * word_len);
				if (!strcpy((char*)word_copy, word, word_len)) {
					// something failed, should never happen
					set_error("ImpossibleErrorOnWordCopy");
					goto force_error;
				}

#ifdef SIMPLE_LANGUAGE_ERROR_MESSAGE
				printf("Line: %i | Word: %s\n", linec, word_copy);
#endif

				SimpleLangObject obj = (SimpleLangObject){
					.ptr = word_copy,
					.size = word_len,
					.id = object_id,
					.type = state,
				};

				word_len = 0;
			}
			word[word_len] = c;
			word_len++;
		}
		i++;
	}

	for (unsigned short j = 0; j < object_list_index; j++) {
		if (object_list[j].id != -1)
			printf("ID:%i|%c|%s\n", object_list[j].id, object_list[j].type, (const char*)object_list[j].ptr);
	}

	objectListFree();
	return;

force_error:
#if SIMPLE_LANGUAGE_ERROR_MESSAGE
	printf("-------------------\nERR:%s\n\tLINE: %i WORD: %s CHAR: %c\n\tLAST STATEs: %c & %c\n", error, linec, word, c, old_state, state);
#else
	printf("-------------------\nERROR(message is disabled)\n\tLINE: %i WORD: %s CHAR: %c\n\tLAST STATEs: %c & %c\n", linec, word, c, old_state, state);
#endif
	objectListFree();
	return;
}
