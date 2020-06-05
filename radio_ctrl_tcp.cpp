#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "cstring"
#include "errno.h"

#include "signal.h"
#include "unistd.h"
#include "fcntl.h"
#include "pthread.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"

#include "gpio.h"
#include "pinmap.h"
#include "common.h"
#include "util.h"

#define UC_PIN_NAME   "uc_ctrl"
#define TX_PIN_NAME   "tx_ctrl"
#define SDR1_PIN_NAME "sdr1_led"
#define SDR2_PIN_NAME "sdr2_led"

#define CTRL_SERVER_MAX_CONN 8
#define CTRL_SERVER_PORT 1233
#define BUFLEN 1024

volatile uint8_t run = 1;
gpio_t* uc_pin = NULL;
gpio_t* tx_pin = NULL;
gpio_t* sdr1_pin = NULL;
gpio_t* sdr2_pin = NULL;

int ctrl_sock_fd = 0;
pthread_t* ctrl_serv_pthr = NULL;

struct client_data_t {
	int sock;
	struct sockaddr_in addr;
	socklen_t alen;
	pthread_t thr;
};
client_data_t* ctrl_serv_clients[CTRL_SERVER_MAX_CONN];

void register_sigs();
void init_gpio(char* name, gpio_t** pin_ptr, uint8_t dir);
void err(char* err);
void start_ctrl_server();
void* ctrl_server_thr(void*);

int main(int argc, char** argv) {
	if(load_pinmap(PINMAP_LOCATION)) {
		init_gpio(UC_PIN_NAME, &uc_pin, GPIO_DIRECTION_OUTPUT);
		init_gpio(TX_PIN_NAME, &tx_pin, GPIO_DIRECTION_OUTPUT);
		init_gpio(SDR1_PIN_NAME, &sdr1_pin, GPIO_DIRECTION_OUTPUT);
		init_gpio(SDR2_PIN_NAME, &sdr2_pin, GPIO_DIRECTION_OUTPUT);
		unload_pinmap();
		puts("Radio control GPIO Initialized!");
	}
	register_sigs();
	start_ctrl_server();
	while(run) usleep(10000);
	return 0;
}

uint8_t get_argc(char* buf) {
	
}
char** get_argv(char* buf) {
	
}
void handle_cmd(client_data_t* c, char* cmd) {
	if(!cmd) return;
	cmd = strtrim(cmd);
	char* err = NULL; 
	char* resp = NULL;
	char buf[BUFLEN]; buf[0] = 0;
	if(strcmp(cmd, "uc") == 0) {
		err = gpio_err_tostr(uc_pin->write(1));
		resp = "Upconverter enabled";
	} else if(strcmp(cmd, "pt") == 0) {
		err = gpio_err_tostr(uc_pin->write(0));
		resp = "Passthrough enabled";
	} else if(strcmp(cmd, "tx") == 0) {
		err = gpio_err_tostr(tx_pin->write(1));
		resp = "TX mode activated";
	} else if(strcmp(cmd, "rx") == 0) {
		err = gpio_err_tostr(tx_pin->write(0));
		resp = "RX mode activated";
	} else if(strcmp(cmd, "clients") == 0) {
		char* tmp = buf + snprintf(buf, BUFLEN, "ACT\n");
		for(uint8_t i = 0; i < CTRL_SERVER_MAX_CONN; i++)
			if(ctrl_serv_clients[i])
				tmp += snprintf(tmp, BUFLEN - strlen(tmp), "%s\n", inet_ntoa(ctrl_serv_clients[i]->addr.sin_addr));
	} else if(strcmp(cmd, "shutdown") == 0) {
		if(system("shutdown -P now"))
			err = "system(\"shutdown -P now\") returned a non-zero value";
		resp = "Shutting down";
	} else if(strcmp(cmd, "status") == 0) {
		
	} else if(strcmp(cmd, "help") == 0) {
		resp = "-------==== Help ====-------\n"
			"help: Shows this menu\n"
			"clients: Shows list of connected clients\n"
			"tx <on/off>: Enable/Disable transmitter switch\n"
			"upcon <on/off>: Enable/Disable upconverter path\n"
			"shutdown: Performs a system shutdown\n"
			"status: Prints system status\n"
			"sdr <num> <on/off>: Enable/disable SDR\n";
	} else err = "Unknown command";
	
	if(!buf[0]) {
		if(err) snprintf(buf, BUFLEN, "NAK\n%s\n", err);
		else if(resp) snprintf(buf, BUFLEN, "ACK\n%s\n", resp);
		else snprintf(buf, BUFLEN, "ACK\n");
	}
	send(c->sock, buf, strlen(buf), 0);
}
void* ctrl_serv_client_thr(void* arg) {
	if(!arg)
		err("ctrl_serv_client_thr called with null argument!");
	char rx_buf[BUFLEN];
	client_data_t** cptr = (client_data_t**) arg;
	client_data_t* c = *cptr;
	if(!c)
		err("ctrl_serv_client_thr called with null client data!");
	if(fcntl(c->sock, F_SETFL, fcntl(c->sock, F_GETFL, 0) | O_NONBLOCK) < 0)
		err("Failed to set non-blocking flag on socket");
	while(run) {
		int r = read(c->sock, rx_buf, BUFLEN); 
		if(r < 0) { //errno is thread safe - this is fine
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(1000);
				continue;
			} else break;
		} else if(r > 0) {
			rx_buf[r] = 0;
			handle_cmd(c, rx_buf);
		} else break;
	}
	close(c->sock);
	printf("Client %s disconnected!\n", inet_ntoa(c->addr.sin_addr));
	free(c); *cptr = NULL;
	return NULL;
}
void* ctrl_server_thr(void* arg) {
	puts("Server starting!");
	if(listen(ctrl_sock_fd, 1) < 0)
		err("Failed to listen on socket!");
	if(fcntl(ctrl_sock_fd, F_SETFL, fcntl(ctrl_sock_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
		err("Failed to set non-blocking flag on socket");
	puts("Server started!");
	while(run) {
		struct sockaddr_in addr; socklen_t addrlen;
		int csock = accept(ctrl_sock_fd, (struct sockaddr*) &addr, &addrlen);
		if(csock < 0) {
			if(errno == EWOULDBLOCK || errno == EAGAIN) {
				usleep(10000);
				continue;
			} else err("Failed to accept TCP connection!");
		}
		uint8_t finished = 0;
		for(uint8_t i = 0; i < CTRL_SERVER_MAX_CONN; i++) {
			if(!ctrl_serv_clients[i]) {
				printf("Client %s [%u] connected!\n", inet_ntoa(addr.sin_addr), i);
				client_data_t* cd = new client_data_t();
				cd->sock = csock;
				cd->addr = addr;
				cd->alen = addrlen;
				ctrl_serv_clients[i] = cd;
				pthread_create(&cd->thr, NULL, ctrl_serv_client_thr, (void*) &ctrl_serv_clients[i]);
				finished = 1;
				break;
			}
		}
		if(finished) continue;
		char* bsy_str = "NAK\nServer is busy!\n";
		send(csock, bsy_str, strlen(bsy_str), 0);
		close(csock);
		printf("Failed to accept TCP Client from %s, too many active connections!\n", inet_ntoa(addr.sin_addr));
	}
	close(ctrl_sock_fd);
	ctrl_sock_fd = 0;
	puts("Server stopped!");
	return NULL;
}
void start_ctrl_server() {
	ctrl_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(!ctrl_sock_fd) 
		err("Failed to create TCP socket!");
	// Set socket re-use address, re-use port
	int opt = 1;
	if(setsockopt(ctrl_sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) 
		err("Failed to set socket options!");
	// Bind socket
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; 
	addr.sin_addr.s_addr = INADDR_ANY; 
	addr.sin_port = htons(CTRL_SERVER_PORT); 
	if(bind(ctrl_sock_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
		err("Failed to bind to socket!");
	// Start server thread
	ctrl_serv_pthr = new pthread_t();
	pthread_create(ctrl_serv_pthr, NULL, ctrl_server_thr, NULL);
}

void err(char* e) {
	puts(e);
	exit(1);
}
void init_gpio(char* name, gpio_t** pin_ptr, uint8_t dir) {
	uint32_t pin;
	if(get_pin(name, &pin)) {
		gpio_t* ptr = new gpio_t(pin);
		gpio_err_t err;
		err = ptr->begin();
		if(err) goto err_lbl;
		err = ptr->set_direction(dir);
		if(err) goto err_lbl;
		*pin_ptr = ptr;
		return;
		
		err_lbl:
		printf("GPIO Error: %s\n", gpio_err_tostr(err));
		exit(1);
	}
}
void on_interrupt(int sig) { run = 0; }
void gpio_finish(gpio_t* p) { if(p) { p->write(0); p->end(); }}
void on_exit() {
	run = 0;
	puts("Radio control TCP server stopping!");
	gpio_finish(uc_pin);
	gpio_finish(tx_pin);
	gpio_finish(sdr1_pin);
	gpio_finish(sdr2_pin);
	puts("Waiting for active connections to close");
	for(uint8_t i = 0; i < CTRL_SERVER_MAX_CONN; i++)
		if(ctrl_serv_clients[i])
			pthread_join(ctrl_serv_clients[i]->thr, NULL);
	puts("Waiting for TCP server to close");
	if(ctrl_sock_fd) {
		pthread_join(*ctrl_serv_pthr, NULL);
		delete ctrl_serv_pthr;
		ctrl_serv_pthr = NULL;
	}
	puts("Radio control TCP server stopped!");
}
void register_sigs() {
	atexit(on_exit);
	signal(SIGINT, on_interrupt);
}
