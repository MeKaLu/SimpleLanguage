#include "simple_language.h"

#include <cstdio>

#define STATE_ZERO						'0'
#define STATE_PIN						'P'
#define STATE_FUNCTION					'F'
#define STATE_ARGUMENT					'A'
#define STATE_COMBINE					'C'
#define STATE_CONDITION					'S'
#define STATE_CONDITION_TYPE		    'T'
#define STATE_CONDITION_ARGUMENT		'B'

#define SPECIAL_COMMENT					'#'
#define SPECIAL_END						';'
#define SPECIAL_COMBINE					'&'
#define SPECIAL_COMBINE_END				'@'
#define SPECIAL_CONDITION				':'
#define SPECIAL_PLACEHOLDER				'+'

#define SPECIAL_END_STR					";\0"

#if SIMPLE_LANGUAGE_ERROR_MESSAGE
#define set_error(message)  strcpy(error, message, SIMPLE_LANGUAGE_ERROR_LEN)
#else
#define set_error(message)
#endif

#pragma pack(1)
struct SimpleLangObject {
	void* ptr;
	char type;
	unsigned char size;
};

SimpleLangObject* object_list = nullptr;
unsigned short object_list_index = 0;
unsigned short object_list_size = 0;

static void objectListInit() {
	object_list_index = 0;
	object_list_size = 10;
	object_list = (SimpleLangObject*)smalloc(sizeof(SimpleLangObject) * object_list_size);
	for (unsigned short i = 0; i < object_list_size; i++)
		object_list[i] = (SimpleLangObject){ 0 };
}

static void objectListFree() {
	for (unsigned short i = 0; i < object_list_size; i++)
		if (object_list[i].ptr != nullptr) sfree(object_list[i].ptr);
	sfree(object_list);
	object_list = nullptr;
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
			object_list[i] = (SimpleLangObject){ 0 };
		} else if (i < oldsize) object_list[i] = oldlist[i];
	}
	sfree(oldlist);
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
	bool dont_append = false;
	bool end_statement = false;
	bool fresh = false;
	bool reject_obj = false;

	while (i < code_size) {
		c = code[i];
		if (c == '\n') {
			if (skip) skip = false;
			else if (!skip) {
				if (word_len > 0) {
					// there should be ';' at the end of each statement but there was not
					set_error("UnfinishedStatement");
					goto force_error;
				}
			}
			linec++;
		} else if (c == '\r' || c == '\t') {
		} else if (c == SPECIAL_COMMENT) {
			skip = true;
		} else if (!skip) {
			// lines cannot start with special characters
			if ((state == STATE_ZERO && old_state == STATE_ZERO) && word_len == 0 && (c == SPECIAL_COMBINE || c == SPECIAL_COMBINE_END || c == SPECIAL_CONDITION || c == SPECIAL_END)) {
				set_error("CannotStartWithSpecialSymbols");
				goto force_error;
			} else if (c == SPECIAL_COMBINE || c == SPECIAL_COMBINE_END || c == SPECIAL_CONDITION || c == SPECIAL_END) {
				if (c == SPECIAL_END && word_len > 0) {
					// word has ended, more like the entire statement ended though
					// so for that to happen, there has to be at least a function
					if (state == STATE_FUNCTION) {
						// it's single(or after the first but not the last) argument function
						dont_append = true;
						end_statement = true;

						old_state = state;
						state = STATE_ARGUMENT;
						goto skip_to_object_append;
					} else if (state == STATE_PIN) {
						// no argument function
						dont_append = true;
						end_statement = true;
						 //fresh start
						fresh = true;

						old_state = state;
						state = STATE_FUNCTION;
						goto skip_to_object_append;
					} else if (state == STATE_ARGUMENT) {
						// multiple(last) argument function
						dont_append = true;
						end_statement = true;

						fresh = true;
						goto skip_to_object_append;
					} else {
						set_error("InvalidUseOfSymbol");
						goto force_error;
					}
				} else if (c == SPECIAL_END && word_len == 0) {
					// space before ';'
					set_error("InvalidUseOfSymbol");
					goto force_error;
				} else if (c == SPECIAL_END && word_len == 0 && state != STATE_FUNCTION) {
					// if it ends with a function and no argument, that is valid but 
					// any other way except ending after the arguments are false
					set_error("CannotStartWithSpecialSymbols");
					goto force_error;
				} else if (c == SPECIAL_COMBINE && word_len == 0) {
					if (state == STATE_PIN && (old_state == STATE_ZERO || old_state == STATE_COMBINE)) {
						// multiple pins
						old_state = state;
						state = STATE_COMBINE;

						// insert placeholder
						word[0] = SPECIAL_PLACEHOLDER;
						word_len = 1;
						dont_append = true;
						goto skip_to_object_append;
					} else if (state == STATE_CONDITION_ARGUMENT && old_state == STATE_CONDITION_TYPE) {
						// multiple conditions
						old_state = STATE_PIN;
						state = STATE_CONDITION;

						dont_append = true;
						goto skip_to_object_append;
					} else {
						// syntax error
						set_error("InvalidUseOfCombine");
						goto force_error;
					} 
				} else if (c == SPECIAL_COMBINE_END) {
					if (old_state != STATE_COMBINE && old_state != STATE_CONDITION_TYPE) {
						// syntax error
						set_error("InvalidUseOfCombineEnd");
						goto force_error;
					}
					dont_append = true;
					// hack, after COMBINE_END there has to be a function
					// so, just act like combine never happened and proceed
					old_state = STATE_ZERO;
					state = STATE_PIN;
				} else if (c == SPECIAL_CONDITION && word_len == 0) {
					if (state == STATE_PIN && old_state == STATE_ZERO) {
						// condition right after pin
						
						old_state = state;
						state = STATE_CONDITION;

						dont_append = true;
						goto skip_to_object_append;
					} else {
						// syntax error
						set_error("InvalidUseOfCondition");
						goto force_error;
					}
				}
			} else if (c == ' ' && word_len > 0) {
				// space before ';'
				if (strcmp(word, SPECIAL_END_STR)) {
					set_error("InvalidUseOfSymbolEnd");
					goto force_error;
				}

				// check if there is no SPECIAL_END at the end of the C & P combos
				if (state == STATE_PIN && old_state == STATE_COMBINE) {
					// check for the last object
					// if it is C & P as well, assume combine ended but no SPECIAL_END
					const SimpleLangObject* o = &object_list[object_list_index - 1];
					if (o->type == STATE_PIN) {
						set_error("ForgotCombineEnd");
						goto force_error;
					}
				} else if (state == STATE_PIN && old_state == STATE_ZERO) {
					// there is going to be a function
					old_state = state;
					state = STATE_FUNCTION;
				} else if (state == STATE_CONDITION && old_state == STATE_PIN) {
					// condition type
					old_state = state;
					state = STATE_CONDITION_TYPE;
				} else if (state == STATE_CONDITION_TYPE && old_state == STATE_CONDITION) {
					// condition
					old_state = state;
					state = STATE_CONDITION_ARGUMENT;
				} else if (state == STATE_FUNCTION && old_state == STATE_PIN) {
					// prevent double functions
					old_state = state;
					state = STATE_ARGUMENT;		
				} else if (state == STATE_PIN && old_state == STATE_ARGUMENT) {
					// statement ended
					old_state = STATE_ZERO;
				} else if (state == STATE_COMBINE && old_state == STATE_PIN) {
					// the other combined pins
					old_state = state;
					state = STATE_PIN;
				}

			skip_to_object_append:
				if (word_len >= SIMPLE_LANGUAGE_MAX_WORD_LEN) {
					set_error("SurpassedMaxWordLength");
					goto force_error;
				}
				word[word_len] = '\0';
				// auto reject
				if (word_len == 0) reject_obj = true;

				// +1 for null character at the end, strcpy will add it
				word_len++;
				void* word_copy = smalloc(sizeof(char) * word_len);
				if (!strcpy((char*)word_copy, word, word_len)) {
					// something failed, should never happen
					set_error("ImpossibleErrorOnWordCopy");
					goto force_error;
				}

#ifdef SIMPLE_LANGUAGE_ERROR_MESSAGE
				printf("Line: %i | Word: '%s' | States: %c & %c\n", linec, (const char*)word_copy, old_state, state);
#endif

				// fresh start
				// that means word HAS to be a pin
				if (state == STATE_ZERO) {
					state = STATE_PIN;
				}

				SimpleLangObject obj = (SimpleLangObject){
					.ptr = word_copy,
					.size = word_len,
					.type = state,
				};

				if (!reject_obj) {
					if (!objectListAppend(obj)) {
						// probably size issue
						// reallocate
						objectListResize(object_list_size + 5);
						// try again
						if (!objectListAppend(obj)) {
							// something went terribly wrong
							set_error("ReallocationObjListFailed");
							// dont forget to free the word_copy
							sfree(word_copy);
							goto force_error;
						}
					}
					dont_append = true;

#ifdef SIMPLE_LANGUAGE_ERROR_MESSAGE
					printf("\tType: %c\n", obj.type);
#endif
				} else {
#ifdef SIMPLE_LANGUAGE_ERROR_MESSAGE
					printf("^rejected\n");
#endif
					sfree(word_copy);
					reject_obj = false;
				} 
				if (end_statement) {
					// fresh start
					if (fresh) {
						old_state = STATE_ZERO;
						state = STATE_ZERO;
						fresh = false;
					} else {
						old_state = state;
						state = STATE_PIN;
					}
					end_statement = false;
				}
				strfill(word, word_len, '\0');
				word_len = 0;
			}

			// dont fucking add the space to the word
			if (!dont_append && c != ' ') {
				word[word_len] = c;
				word_len++;
			} else dont_append = false;
		}
		i++;
	}

	{
		short size = 0;
		unsigned short j = 0;
		for (; j < object_list_index; j++) {
			printf("%c|%s\n", object_list[j].type, (const char*)object_list[j].ptr);
			size += sizeof(SimpleLangObject);
			size += object_list[j].size;
		}
		printf("Size: %i, total amount of obj: %u\n", size, j);
	};

	objectListFree();
	return;

force_error:
#if SIMPLE_LANGUAGE_ERROR_MESSAGE
	printf("-------------------\nERR:%s\n\tLINE: %i WORD: '%s' CHAR: '%c' \n\tLAST STATEs: %c & %c\n", error, linec, word, c, old_state, state);
#else
	printf("-------------------\nERROR(message is disabled)\n\tLINE: %i WORD: '%s' CHAR: '%c' \n\tLAST STATEs: %c & %c\n", linec, word, c, old_state, state);
#endif
	objectListFree();
	return;
}
