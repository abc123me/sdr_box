#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"

#include "signal.h"
#include "unistd.h"

#include "gpio.h"
#include "pinmap.h"
#include "common.h"
#include "util.h"

#define PIN_NAME "fan_ctrl"
#define BUFLEN 128

#define _GPIO_CHK(code) \
	err = code;\
	if(err) {\
		printf("GPIO Error: ");\
		puts(gpio_err_tostr(err));\
		return 1;\
	}

gpio_t* gpio_pin = NULL;
FILE* tfile;
char tbuf[BUFLEN];
volatile uint8_t loop = 1;

void register_sigs();
void load_pin(uint32_t* pin);

int main(int argc, char** argv) {
	tfile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	if(!tfile) {
		puts("Failed to open /sys/class/thermal/thermal_zone0/temp, cannot monitor temperature!");
		return 1;
	}
	setbuf(tfile, NULL);
	
	uint32_t pin;
	load_pin(&pin);
	gpio_err_t err;
	gpio_pin = new gpio_t(pin);
	_GPIO_CHK(gpio_pin->begin())
	_GPIO_CHK(gpio_pin->set_direction(GPIO_DIRECTION_OUTPUT))
	register_sigs();

	uint8_t i = 0;
	int8_t dc = 0;
	float temp;
	while(loop) {
		if(i == 0) {
			fseek(tfile, 0, SEEK_SET);
			fgets(tbuf, BUFLEN, tfile);
			char* d = strtrim(tbuf);
			float temp = atof(d) / 1000.0f;
			dc = ceil(((temp - 40.0f) / 15.0f) * 10.0f);
			if(dc > 10) dc = 10;
			if(dc < 0) dc = 0;
			//printf("%.1f, %u\n", temp, dc);
		}
		_GPIO_CHK(gpio_pin->write(i < dc ? 1 : 0))
		if(++i >= 10) i = 0;
		usleep(10000);
	}

	fclose(tfile);
	tfile = NULL;

	return 0;
}

void load_pin(uint32_t* pin) {
	uint8_t ec = 0;
	if(load_pinmap(PINMAP_LOCATION)) {
		if(get_pin(PIN_NAME, pin)) printf("Found pin %u for %s\n", *pin, PIN_NAME); 
		else { puts("Failed to load pin from pin map!"); ec = 1; }
		unload_pinmap();
	}
	if(ec) exit(ec);
}
void on_interrupt(int sig) {
	loop = 0;
}
void on_exit() {
	if(gpio_pin) {
		gpio_pin->write(0);
		gpio_pin->end();
		free(gpio_pin);
	}
	if(tfile) fclose(tfile);
	puts("Fan control script stopped!");
}
void register_sigs() {
	atexit(on_exit);
	signal(SIGINT, on_interrupt);
}
