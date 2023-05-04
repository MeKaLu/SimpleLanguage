#include "simple_language.h"

#include <stdbool.h>

/*
	P -> pin
	F -> func
	A -> params
	C -> combine	[C|n(P)|F|n(A)]
	S -> condition	[P<n(S)>|F|n(A)] or [C|n(P)<n(S)>|F|n(A)]
*/
#define STATE_PIN			'P'
#define STATE_FUNCTION		'F'
#define STATE_ARGUMENT		'A'
#define STATE_COMBINE		'C'
#define STATE_CONDITION		'S'

#define SPECIAL_COMMENT		'#'
#define SPECIAL_END			';'
#define SPECIAL_COMBINE		'@'
#define SPECIAL_CONDITION	':'

#define set_error(message)  strcpy(error, message, SIMPLE_LANGUAGE_ERROR_LEN)

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
	unsigned short linec = 0;
	char c = 0;

	char error[SIMPLE_LANGUAGE_ERROR_LEN];
	set_error("UNKOWN");

	unsigned char word_i = 0;
	char word[SIMPLE_LANGUAGE_MAX_WORD_LEN];
	strfill(word, SIMPLE_LANGUAGE_MAX_WORD_LEN, '\0');
	
	char state = STATE_PIN;
	bool multiple = false;
	bool end = false;
	bool skip = false;

	while (i < code_size) {
		c = code[i];
		if (c == '\n') {
			linec++;
			if (skip) skip = false;
			else if (!skip && state == STATE_ARGUMENT) {
				set_error("INVALIDSTATEARGUMENT");
				goto force_error;
			}
		} else if (c == '\r' || c == '\t') {
		} else if (c == SPECIAL_COMMENT) {
			skip = true;
		} else {
			bool dont_append = false;
			if (!skip) {
				if (c == ' ') {
					dont_append = true;
				force_object:
					// rejection can still happen even if the object is forced
					bool reject = false;
					word[word_i] = '\0';

					void* word_copy = smalloc(sizeof(char) * word_i + 1);
					strcpy((char*)word_copy, word, word_i + 1);

					short id = word_i + ((i != 0) ? i / 10 : 0);
					while (objectListFind(id) != -1) id++;
					SimpleLangObject obj = (SimpleLangObject){
						.ptr = word_copy,
						.size = word_i + 1,
						.id = id,
						.type = state,
					};

					printf("%s\n", (const char*)word_copy);

					if (state == STATE_PIN) {
						// 'word_i > 0' hardcoded to avoid empty objects
						// if 'word_i == 0' it will reject the object append 
						if (!multiple && word_i > 0) {
							state = STATE_FUNCTION;
						} else {
							// hardcoded to avoid empty objects
							if (word_i == 0) reject = true; // possible need of escape here, or else it may interfere
						}
					} else if (state == STATE_FUNCTION) {
						// check if function exists
						// hardcoded, [C|n(P)|F(empty)|A(F is here)|n(A)]
						// avoided by doing this
						if (word_i == 0) reject = true;
						else {
							state = STATE_ARGUMENT;
						}
					} else if (state == STATE_ARGUMENT) {
						if (end) {
							end = false;
							state = STATE_PIN;
						}
						// check if param exists in the function
					} else if (state == STATE_COMBINE) {
						// Combine
						multiple = true;
						state = STATE_PIN;
					}

					if (reject == false) {
						if (!objectListAppend(obj)) {
							objectListResize(object_list_size + 5);
							if (!objectListAppend(obj)) {
								goto force_error;
							}
						}
					} else {
						sfree(word_copy);
					}
					word_i = 0;
				} else if (c == SPECIAL_COMBINE) {
					if (state != STATE_PIN) {
						set_error("INVALIDSTATE");
						goto force_error;
					}
					else if (!multiple) {
						state = STATE_COMBINE;
						dont_append = true;
					} else {
						state = STATE_FUNCTION;
						multiple = false;
						dont_append = true;
					}
				} else if (c == SPECIAL_END) {
					if (state == STATE_ARGUMENT) {
						dont_append = true;
						end = true;
						goto force_object;
					} else if (state == STATE_PIN && multiple) {
						dont_append = true;
						goto force_object;
					} else {
						set_error("INVALIDSTATECOMBINE");
						goto force_error;
					}
				}

				if (!dont_append) {
					word[word_i] = c;
					word_i++;
				}
			}
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
	printf("-------------------\nERR:%s\n\tLINE: %i WORD: %s\n\tLAST STATE: %c\n", error, linec, word, state);
	objectListFree();
	return;
}
