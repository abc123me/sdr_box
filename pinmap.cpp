#include "pinmap.h"

#include "stdio.h"
#include "stdlib.h"
#include "cstring"

#include "util.h"

static FILE* fp = NULL;

uint8_t load_pinmap(char* loc) {
	fp = fopen(loc, "r");
	return fp ? 1 : 0;
}
void unload_pinmap() {
	fclose(fp);
}

uint8_t get_pin(char* name, uint32_t* def) {
	fseek(fp, 0, SEEK_SET);
	char buf[128];
	while(fgets(buf, 128, fp)) {
		char* begin = buf;
		char* end = buf;
		while(*end && *end != '=') end++;
		end[0] = 0;
		begin = strtrim(begin);
		end = strtrim(end + 1);
		if(strcmp(name, begin) == 0 && numeric(end)) {
			*def = atoi(end);
			return 1;
		}
	}
	return 0;
}
