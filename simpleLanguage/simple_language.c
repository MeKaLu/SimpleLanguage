#include "simple_language.h"

#include <stdbool.h>

/*
	P -> pin
	F -> func
	A -> params
	C -> combine	[C|n(P)|C|F|n(A)]
	S -> condition	[P|S|n(B)|F|n(A)] or [C|n(P)|S|n(B)|F|n(A)]
*/
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

#if SIMPLE_LANGUAGE_ERROR_MESSAGE
	char error[SIMPLE_LANGUAGE_ERROR_LEN];
#endif
	set_error("UNKOWN");

	unsigned char word_i = 0;
	char word[SIMPLE_LANGUAGE_MAX_WORD_LEN];
	strfill(word, SIMPLE_LANGUAGE_MAX_WORD_LEN, '\0');
	
	char old_state = STATE_PIN;
	char state = STATE_PIN;
	bool end = false;
	bool skip = false;

	while (i < code_size) {
		c = code[i];
		if (c == '\n') {
			linec++;
			if (skip) skip = false;
			else if (!skip && state == STATE_ARGUMENT) {
				// there should be ';' at the end of each statement but there was not
				set_error("UNFINISHEDSTATEMENT");
				goto force_error;
			}
		} else if (c == '\r' || c == '\t') {
		} else if (c == SPECIAL_COMMENT) {
			skip = true;
		} else {
			bool dont_append = false;
			if (!skip) {
				// spaces are very important in this language
				if (c == ' ') {
					dont_append = true;
				force_object:
					// rejection can still happen even if the object is forced
					bool reject = false;
					word[word_i] = '\0';

					// if there is a rejection it will be freed at the end,
					// else it will be freed when object list is getting freed
					void* word_copy = smalloc(sizeof(char) * word_i + 1);
					if (!strcpy((char*)word_copy, word, word_i + 1)) {
						// word was longer than word_i
						goto force_error;
					}

					// create somewhat random word id, there should be no possible 
					// same id created here but just in case it will check and act accordingly
					short id = word_i + ((i != 0) ? i / 10 : 0);
					while (objectListFind(id) != -1) id++;
					SimpleLangObject obj = (SimpleLangObject){
						.ptr = word_copy,
						.size = word_i + 1,
						.id = id,
						.type = state,
					};

#if SIMPLE_LANGUAGE_ERROR_MESSAGE
					printf("%s\n", (const char*)word_copy);
#endif

					if (state == STATE_PIN) {
						// 'word_i > 0' hardcoded to avoid empty objects
						// if 'word_i == 0' it will reject the object append
						// otherwise pin will slide into the function and function will slide into argument
						if (word_i > 0) {
							if (old_state == STATE_COMBINE) {
								// [C] is empty
							} else {
								old_state = state;
								state = STATE_FUNCTION;
							}
						} else {
							// hardcoded to avoid empty objects
							if (word_i == 0) reject = true; // possible need of escape here, or else it may interfere
							// multiple pins
							// old state should be COMBINE
							else if (old_state != STATE_COMBINE) {
								set_error("UNKNOWNUSEOFPINS");
								goto force_error;
							} else goto force_error;
						}
					} else if (state == STATE_FUNCTION) {
						// check if function exists
						// hardcoded, [C|n(P)|F(empty)|A(F is here)|n(A)]
						// sliding the object types avoided with this
						if (word_i == 0) reject = true;
						else {
							old_state = state;
							state = STATE_ARGUMENT;
						}
					} else if (state == STATE_ARGUMENT) {
						if (old_state == STATE_FUNCTION) {
							// check if param exists in the function
							if (end) {
								// add the argument then reset the state
								end = false;
								old_state = state;
								state = STATE_PIN;
							}
						} else {
							set_error("UNKNOWNUSEOFARGUMENTS");
							goto force_error;
						}
					} else if (state == STATE_COMBINE) {
						// Combine
						old_state = state;
						state = STATE_PIN;
					} else if (state == STATE_CONDITION) {
						// hardcoded to avoid empty objects
						if (word_i == 0) reject = true; // possible need of escape here, or else it may interfere
						else {
							old_state = state;
							state = STATE_CONDITION_ARGUMENT;
						}
					} else if (state == STATE_CONDITION_ARGUMENT) {
						// hardcoded to avoid empty objects
						if (word_i == 0) reject = true; // possible need of escape here, or else it may interfere
						else if (old_state == STATE_CONDITION) {
							// after a condition arg, there should be a function 
							// but there might also be a combined condition
							// either way, we are going to assume there will be a function
							old_state = state;
							state = STATE_FUNCTION;
						} else {
							set_error("UNKNOWNUSEOFCONDITIONARGUMENT");
							goto force_error;
						}
					} else {
						set_error("UNKNOWNSTATE");
						goto force_error;
					}

					if (reject == false) {
						if (!objectListAppend(obj)) {
							objectListResize(object_list_size + 5);
							if (!objectListAppend(obj)) {
								// there should be no possible way it will reach this but 
								// just in case we run out of memory and/or malloc failed
								set_error("FAILEDTOAPPENDOBJECT");
								// free the word_copy, we are aborting the function
								sfree(word_copy);
								goto force_error;
							}
						}
					} else {
						sfree(word_copy);
					}
					word_i = 0;
				} else if (c == SPECIAL_COMBINE) {
					// after [C] there is going to be [P]
					if (state == STATE_PIN) {
						// start adding the pins
						old_state = state;
						state = STATE_COMBINE;
						dont_append = true;
					} else if (state == STATE_PIN && old_state == STATE_COMBINE) {
						// end
						old_state = state;
						state = STATE_FUNCTION;
						dont_append = true;
					} else if (state == STATE_FUNCTION && old_state == STATE_CONDITION_ARGUMENT) {
						// multiple conditions, [pin] : < 500 @ > 0 :
						// after a condition argument it is assumened to have a function
						// but in this case not
						old_state = state;
						state = STATE_CONDITION;
						dont_append = true;
					} else {
						set_error("INVALIDUSEOFCOMBINE");
						goto force_error;
					}
				} else if (c == SPECIAL_CONDITION) {
					// there is always pin before a condition
					// but after a pin state becomes function
					// so we check for the last state
					if (state == STATE_FUNCTION && old_state == STATE_PIN) {
						old_state = state;
						state = STATE_CONDITION;
						dont_append = true;
					} else if (state == STATE_PIN && old_state == STATE_COMBINE) {
						// combine just started therefore current state is pin
						// multiple is also true
						old_state = state;
						state = STATE_CONDITION;
						dont_append = true;
					} else if (state == STATE_FUNCTION && old_state == STATE_CONDITION_ARGUMENT) {
						// condition ended
						old_state = state;
						state = STATE_FUNCTION;
						dont_append = true;
					} else {
						set_error("INVALIDUSEOFCONDITION");
						goto force_error;
					}
				} else if (c == SPECIAL_END) {
					if (state == STATE_ARGUMENT) {
						// add the argument then reset the state
						dont_append = true;
						end = true;
						goto force_object;
					} else if (state == STATE_PIN && old_state == STATE_COMBINE) {
						// add the argument then continue for the others
						dont_append = true;
						goto force_object;
					} else if (state == STATE_FUNCTION) {
						// no arguments for the function
						// add the function and reset the state
						dont_append = true;
						end = true;
						// we do this so it is not going to report an error after '\n' in the source code
						old_state = state;
						state = STATE_ARGUMENT;
						goto force_object;
					} else {
						// probably just written ';' otherwise there is no way to get here
						set_error("UNKOWNENDING");
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
#if SIMPLE_LANGUAGE_ERROR_MESSAGE
	printf("-------------------\nERR:%s\n\tLINE: %i WORD: %s\n\tLAST STATEs: %c & %c\n", error, linec, word, old_state, state);
#else
	printf("-------------------\nERROR(message is disabled)\n\tLINE: %i WORD: %s\n\tLAST STATEs: %c & %c\n", linec, word, old_state, state);
#endif
	objectListFree();
	return;
}
