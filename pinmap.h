#ifndef _PINMAP_H
#define _PINMAP_H

#include "stdint.h"

uint8_t load_pinmap(char* loc);
void unload_pinmap();
uint8_t get_pin(char* name, uint32_t* def);

#endif
