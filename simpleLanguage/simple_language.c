#include "simple_language.h"

#include <stdbool.h>

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

static void _objectListResize(unsigned short newsize) {
	SimpleLangObject* oldlist = object_list;

	const unsigned int oldsize = object_list->size;
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
	
	bool skip = false;
	/*
		P -> pin
		F -> func
		A -> params
		C -> combine
	*/
	char state = 'P';

	while (i < code_size) {
		c = code[i];
		if (c == '\n' && skip) skip = false;
		else if (c == '\n' || c == '\r' || c == '\t') {
		} else if (c == '#') {
			skip = true;
		} else {
			if (!skip) {
				/*
					P -> pin
					F -> func
					A -> params
					C -> combine
				*/
				if (c == ' ') {
					word[word_i] = '\0';
					void* word_copy = smalloc(sizeof(char) * word_i + 1);
					strcpy((char*)word_copy, word, word_i + 1);

					short id = word_i + ((i != 0) ? i / 10 : 0);
					while (_objectListFind(id) != -1) id++;
					SimpleLangObject obj = (SimpleLangObject){
						.ptr = word_copy,
						.size = word_i + 1,
						.id = id,
					};

					printf("%s\n", (const char*)word_copy);
				tryagain:
					if (!_objectListAppend(obj)) {
						_objectListResize(object_list->size + 5);
						goto tryagain;
					}
				} else if (c == ';') {

				}
				word[word_i] = c;
				word_i++;
			}
		}
		i++;
	}

	for (unsigned short i = 0; i < object_list_index; i++) {
		if (object_list[i].id != -1)
			printf("ID:%i|%s\n", object_list[i].id, (const char*)object_list[i].ptr);
	}

	_objectListFree();
}
