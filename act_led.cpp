#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"

#include "signal.h"
#include "unistd.h"

#include "gpio.h"
#include "pinmap.h"
#include "common.h"

#define PIN_NAME "act_led"

#define _GPIO_CHK(code) \
	err = code;\
	if(err) {\
		printf("GPIO Error: ");\
		puts(gpio_err_tostr(err));\
		return 1;\
	}

gpio_t* gpio_pin = NULL;
volatile uint8_t loop = 1;

void register_sigs();
void load_pin(uint32_t* pin);

int main(int argc, char** argv) {
	uint32_t pin;
	load_pin(&pin);
	
	gpio_err_t err;
	gpio_pin = new gpio_t(pin);
	_GPIO_CHK(gpio_pin->begin())
	_GPIO_CHK(gpio_pin->set_direction(GPIO_DIRECTION_OUTPUT))
	register_sigs();
	
	uint8_t i = 0;
	while(loop) {
		_GPIO_CHK(gpio_pin->write(i = ~i))
		usleep(100000);
	}
	
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
	puts("SCT LED script stopping!");
	if(gpio_pin) {
		gpio_pin->write(0);
		gpio_pin->end();
		free(gpio_pin);
	}
	puts("SCT LED script stopped!");
}
void register_sigs() {
	atexit(on_exit);
	signal(SIGINT, on_interrupt);
}
