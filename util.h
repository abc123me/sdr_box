#ifndef _UTIL_H
#define _UTIL_H

#include "stdint.h"
#include "stdio.h"

uint16_t get_threads();

#define is_ws(c) (c <= ' ' || c > '~')
#define is_num(c) (c >= '0' && c <= '9')
#define is_numf(c) (is_num(c) || c == '.' || c == '+' || c == '-' || c == 'e' || c == 'E')

uint8_t str_startswith(char* str, char* with);
uint8_t compare_arg(char* arg, char* lng, char* sht);
uint8_t numeric(char* str);
char* malloc_str(uint32_t len, char c);
char* strtrim(char* str);
int32_t indexof(char* str, char of);
int32_t indexofany(char* str, char* anyof);
char* upper(char* s);
char* lower(char* s);

template<typename T> T clamp01(T v) {
	if(v > 1) return 1;
	if(v < 0) return 0;
	return v;
};
template<typename T> T abs(T v) {
	if(v < 0) return -v;
	return v;
}

uint64_t fileSize(FILE* fp);
char* readFile(FILE* fp);
void readFileInto(FILE* fp, char* buf);

void rotateArray(void* array, unsigned long int arraySize, int by);

template <typename T> inline T map(T val, T min, T max, T nmin, T nmax) {
	return nmin + (((val - min) / (max - min)) * (nmax - nmin));
}
template <typename T> T wrap(T val, T min, T max){
	if(val < min)
		return (max - min) - (val - min);
	if(val > max)
		return (val - min);
	return val + min;
}

#endif
