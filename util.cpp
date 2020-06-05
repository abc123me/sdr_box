#include "util.h"

#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "pthread.h"
#include "math.h"

// String stuff
uint8_t numeric(char* str) {
	for(uint16_t i = 0; i < strlen(str); i++)
		if(!is_num(str[i])) return 0;
		return 1;
}
char* upper(char* in) {
	for(uint16_t i = 0; i < strlen(in); i++)
		if(in[i] >= 'a' && in[i] <= 'z')
			in[i] ^= 32;
		return in;
}
char* lower(char* in) {
	for(uint16_t i = 0; i < strlen(in); i++)
		if(in[i] >= 'A' && in[i] <= 'Z')
			in[i] |= 32;
		return in;
}
int32_t indexof(char* str, char of) {
	for(uint16_t i = 0; i < strlen(str); i++)
		if(str[i] == of) return i;
		return -1;
}
int32_t indexofany(char* str, char* of) {
	uint16_t of_len = strlen(of), str_len = strlen(str);
	for(uint16_t i = 0; i < str_len; i++)
		for(uint16_t j = 0; j < of_len; j++)
			if(str[i] == of[j])
				return i;
			return -1;
}
char* strtrim(char* str) {
	char c = *str;
	while(c && is_ws(c))
		c = *(++str);
	uint16_t len = strlen(str);
	if(!len) return str;
	char* end = str + len - 1;
	while(is_ws(*end))
		end--;
	*++end = 0;
	return str;
}
uint8_t str_startswith(char* str, char* with) {
	uint16_t len = strlen(with);
	if(len > strlen(str))
		return 0;
	return strncmp(str, with, len) == 0;
}
uint8_t compare_arg(char* str, char* with, char* shorthand) {
	uint16_t len = strlen(str);
	if(len < (shorthand ? strlen(shorthand) : strlen(with)))
		return 0;
	return (strcmp(str, with) == 0) || (shorthand && (strcmp(str, shorthand) == 0));
}
char* malloc_str(uint32_t len, char c) {
	char* out = (char*) malloc(len + 1);
	for(uint32_t i = 0; i < len; i++)
		out[i] = c;
	out[len] = 0;
	return out;
}

// File IO
char* readFile(FILE* fp){
	uint64_t size = fileSize(fp);
	char* dat = new char[size + 1];
	readFileInto(fp, dat);
	dat[size] = 0x00;
	return dat;
}
void readFileInto(FILE* fp, char* dat){
	uint64_t size = fileSize(fp);
	fseek(fp, 0L, SEEK_SET);
	fread(dat, 1, size, fp);
}
uint64_t fileSize(FILE* fp){
	fseek(fp, 0L, SEEK_SET);
	fseek(fp, 0L, SEEK_END);
	uint64_t fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return fileSize;
}

// Miscellaneous
#ifdef __linux__
#include "unistd.h"
uint16_t get_threads() { return sysconf(_SC_NPROCESSORS_ONLN);}
#else
#pragma warn OS Not supported, get_threads() wont work properly!
uint16_t get_threads() { return 2; }
#endif
void rotateArray(void* array, unsigned long int size, int by){
	void* newArray = alloca(size);
	for(unsigned int i = 0; i < size; i++){
		long int k = i + by;
		if(k >= size)
			k -= size;
		if(k < 0)
			k += size;
		((uint8_t*)newArray)[i] = ((uint8_t*)array)[k];
	}
	memcpy(array, newArray, size);
}
