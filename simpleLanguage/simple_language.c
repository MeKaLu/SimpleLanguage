#include "simple_language.h"

#include <stdbool.h>

enum SimpleLangOperator {
	SIMPLE_LANG_OP_ASSIGNMENT,
};

enum SimpleLangCondition {
	SIMPLE_LANG_COND_GREATER,
	SIMPLE_LANG_COND_LESSER,
	SIMPLE_LANG_COND_EQGREATER,
	SIMPLE_LANG_COND_EQLESSER,
	SIMPLE_LANG_COND_EQUAL,
	SIMPLE_LANG_COND_NOTEQUAL,
};

struct SimpleLangObject {
	void* ptr;
	const void* func;
	
	unsigned char size;
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
		sfree(object_list[i].ptr);
	sfree(object_list);
	object_list = NULL;
	object_list_index = 0;
	object_list_size = 0;
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

static void strcpy(char* dest, const char* source, unsigned short size) {
	for (unsigned short i = 0; i < size; i++)
		dest[i] = source[i];
}

static void strfill(char* str, unsigned short size, const char fill) {
	for (unsigned short i = 0; i < size; i++)
		str[i] = fill;
}

void simpleLangExecute(const char* code, const unsigned short code_size) {
	_objectListInit();

	unsigned short i = 0;
	unsigned char c = 0;
	unsigned short line = 1;
	bool skip_line = false;

	unsigned char word_i = 0;
	char word[100];
	strfill(word, 100, '\0');

	while (i < code_size) {
		c = code[i];
		if (c == '#') {
			skip_line = true;
		} else if (!skip_line && c == ' ') {
			word[word_i] = '\0';

			char* ptr = (char*)smalloc(sizeof(char) * word_i);
			strcpy(ptr, word, word_i);

			short id = line + word_i;
			if (_objectListFind(id) != -1) {
				while (_objectListFind(id) != -1) id += 1;
			}
			SimpleLangObject obj = (SimpleLangObject){
				.id = id,
				.ptr = ptr,
			};

			word_i = 0;
			sprint(word);
			sprint("\n");

			if (!_objectListAppend(obj)) break;

		} else if (c == '\n') {
			line++;
			skip_line = false;
		} else if (!skip_line) {
			word[word_i] = c;
			word_i++;
		}
		i++;
	}

	for (unsigned short i = 0; i < object_list_index; i++) {
		if (object_list[i].id != -1)
			printf("ID:%i|%s\n", object_list[i].id, (const char*)object_list[i].ptr);
	}

	_objectListFree();
}
