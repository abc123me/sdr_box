#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "signal.h"
#include "cstring"

#include "unistd.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "linux/sockios.h"
#include "linux/if.h"
#include "linux/ethtool.h"

#include "gpio.h"
#include "pinmap.h"
#include "common.h"
#include "util.h"

#define PIN_NAME "net_led"
#define INTERFACE_DONT_EXIST	255
#define INTERFACE_DOWN			0
#define INTERFACE_UP			1
#define INTERFACE_UNKNOWN		2

#define _GPIO_CHK(code) \
	err = code;\
	if(err) {\
		printf("GPIO Error: ");\
		puts(gpio_err_tostr(err));\
		return 1;\
	}

gpio_t* gpio_pin = NULL;
uint16_t speed = 0;
volatile uint8_t loop = 1;

uint16_t getSpeed(char* iface);
void register_sigs();
void load_pin(uint32_t* pin);

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("Usage: %s <interface>\n", argv[0]);
		return 1;
	}
	uint32_t pin;
	load_pin(&pin);
	
	gpio_err_t err;
	gpio_pin = new gpio_t(pin);
	_GPIO_CHK(gpio_pin->begin())
	_GPIO_CHK(gpio_pin->set_direction(GPIO_DIRECTION_OUTPUT))
	register_sigs();
	
	uint8_t i = 0, led = 0;
	uint16_t if_speed;
	while(loop) {
		if(i == 0) if_speed = getSpeed(argv[1]);
		if(if_speed < 1) { 							led = 0;	} //if speed < 1Mbps (0Mbps) - Stay off
		else if(if_speed < 100) { if(i % 5 == 0)	led = !led;	} //if speed < 100Mbps (1/10Mbps) - Blink every 500ms
		else if(if_speed < 1000) { if(i % 2 == 0)	led = !led;	} //if speed < 1Gbps (100Mbps) - Blink every 100ms
		else {										led = 1;	} //if speed >= 1Gbps - Stay on
		gpio_pin->write(led);

		i = i >= 9 ? 0 : i + 1;
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
void on_interrupt(int sig) { loop = 0; }
void on_exit() {
	puts("Network LED script stopping!");
	if(gpio_pin) {
		gpio_pin->write(0);
		gpio_pin->end();
		free(gpio_pin);
	}
	puts("Network LED script stopped!");
}
void register_sigs() {
	atexit(on_exit);
	signal(SIGINT, on_interrupt);
}

uint8_t getInterfaceState(char* iface) {
	FILE* fp = fopen("/sys/class/net/eth0/operstate", "r");
	if(!fp) return INTERFACE_DONT_EXIST;
	char* buf = (char*) alloca(16);
	fgets(buf, 16, fp);
	uint8_t out = INTERFACE_UNKNOWN;
	buf = strtrim(buf);
	if(strcmp("up", buf) == 0) out = INTERFACE_UP;
	else if(strcmp("down", buf) == 0) out = INTERFACE_DOWN;
	fclose(fp);
	return out;
}
uint16_t getSpeed(char* iface) {
	if(getInterfaceState(iface) != INTERFACE_UP)
		return 0;
	
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	struct ifreq ifr;
	struct ethtool_cmd edata;
	strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name));
	ifr.ifr_data = &edata;
	edata.cmd = ETHTOOL_GSET;
	
	if(ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
		perror("ioctl");
		exit(1);
	}
	
	ethtool_cmd_speed(&edata);
	
	close(sock);
	return edata.speed;
}
