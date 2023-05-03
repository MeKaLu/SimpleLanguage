#include "simple_language.h"

#include <stdbool.h>

struct SimpleLangObject {
	void* ptr;
	const void* func;
	
	unsigned char size;
	unsigned char type;
	short id;
};
typedef struct SimpleLangObject SimpleLangObject;

SimpleLangObject* object_list = NULL;
unsigned short object_list_index = 0;
unsigned short object_list_size = 0;

static void _objectListInit() {
	object_list_index = 0;
	object_list_size = 10;
	object_list = (SimpleLangObject*)smalloc(sizeof(SimpleLangObject) * object_list_size);
	for (unsigned short i = 0; i < object_list_size; i++)
		object_list[i] = (SimpleLangObject){ NULL, NULL, 0 , -1 };
}

static void _objectListFree() {
	for (unsigned short i = 0; i < object_list_size; i++)
		if (object_list[i].ptr != NULL) sfree(object_list[i].ptr);
	sfree(object_list);
	object_list = NULL;
	object_list_index = 0;
	object_list_size = 0;
}

static void _objectListResize(unsigned short newsize) {
	SimpleLangObject* oldlist = object_list;

	const unsigned int oldsize = object_list_size;
	object_list_size = newsize;
	
	object_list = (SimpleLangObject*)smalloc(sizeof(SimpleLangObject) * object_list_size);
	for (unsigned short i = 0; i < object_list_size; i++) {
		if (i > oldsize) {
			object_list[i] = (SimpleLangObject){ NULL, NULL, 0 , -1 };
		} else if (i < oldsize) object_list[i] = oldlist[i];
	}
	sfree(oldlist);
}

// returns the position of the obj
static short _objectListFind(short id) {
	for (unsigned short i = 0; i < object_list_index; i++)
		if (object_list[i].id == id) return i;
	return -1;
}

static bool _objectListAppend(SimpleLangObject obj) {
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

static void strcpy(char* dest, const char* source, unsigned short size) {
	const unsigned short so_len = strlen(source);
	for (unsigned short i = 0; i < size; i++) {
		if (i >= so_len) dest[i] = '\0';
		else dest[i] = source[i];
	}
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
	_objectListInit();

	unsigned short i = 0;
	unsigned char c = 0;

	unsigned char word_i = 0;
	char word[100];
	strfill(word, 100, '\0');
	
	/*
		P -> pin
		F -> func
		A -> params
		C -> combine
	*/
	unsigned char state = 'P';
	bool multiple = false;
	bool end = false;

	bool skip = false;

	while (i < code_size) {
		c = code[i];
		if (c == '\n' && skip) skip = false;
		else if (c == '\n' || c == '\r' || c == '\t') {
		} else if (c == '#') {
			skip = true;
		} else {
			bool dont_append = false;
			if (!skip) {
				/*
					P -> pin
					F -> func
					A -> params
					C -> combine [C|n(P)|F|n(A)]
				*/
				if (c == ' ') {
					bool reject = false;
					dont_append = true;
				force_object:
					word[word_i] = '\0';

					void* word_copy = smalloc(sizeof(char) * word_i + 1);
					strcpy((char*)word_copy, word, word_i + 1);

					short id = word_i + ((i != 0) ? i / 10 : 0);
					while (_objectListFind(id) != -1) id++;
					SimpleLangObject obj = (SimpleLangObject){
						.ptr = word_copy,
						.size = word_i + 1,
						.id = id,
						.type = state,
					};

					// printf("%s\n", (const char*)word_copy);

					if (state == 'P') {
						if (!multiple) {
							state = 'F';
						} else {

						}
					} else if (state == 'F') {
						// check if function exists
						// hardcoded, [C|n(P)|F(empty)|A(F is here)|n(A)]
						// avoided by doing this
						if (word_i == 0) reject = true;
						else {
							state = 'A';
						}
					} else if (state == 'A') {
						if (end) {
							end = false;
							state = 'P';
						}
						// check if param exists in the function
					} else if (state == 'C') {
						// Combine
						multiple = true;
						state = 'P';
					}

					if (reject == false) {
						if (!_objectListAppend(obj)) {
							_objectListResize(object_list_size + 5);
							_objectListAppend(obj);
						}
					} else {
						sfree(word_copy);
					}
					word_i = 0;
				} else if (c == '@') {
					if (state != 'P') goto invalid_statement;
					else if (!multiple) {
						state = 'C';
						dont_append = true;
					} else {
						state = 'F';
						multiple = false;
						dont_append = true;
					}
				} else if (c == ';') {
					if (state == 'A') {
						// end of the statement
						word[word_i] = ' ';
						word_i++;
						dont_append = true;
						end = true;
						goto force_object;
					} else if (state == 'P' && multiple) {
						dont_append = true;
						goto force_object;
					} else goto invalid_statement;
				}

				if (!dont_append) {
					word[word_i] = c;
					word_i++;
				}
			}
		}
		i++;
	}

	for (unsigned short i = 0; i < object_list_index; i++) {
		if (object_list[i].id != -1)
			printf("ID:%i|%c|%s\n", object_list[i].id, object_list[i].type, (const char*)object_list[i].ptr);
	}

	_objectListFree();
	return;

invalid_statement:
	sprint("ERR:INVALID_STATEMENT\n");
	_objectListFree();
	return;
}
