OBJS=gpio.o pinmap.o util.o
DEPS=pthread
CFLAGS=-lpthread -Wno-write-strings -Wno-pointer-arith
CC=g++

all:	act_led net_led rctl_server fan_ctrl clean_objs

clean_objs:
	rm -fv *.o
clean: clean_objs
	rm -fv bin/*

%.o: %.cpp
	$(CC) -c $< -o $@ $(CFLAGS)
net_led: $(OBJS)
	$(CC) $(CFLAGS) net_led.cpp $(OBJS) -o bin/net_led
act_led: $(OBJS)
	$(CC) $(CFLAGS) act_led.cpp $(OBJS) -o bin/act_led
fan_ctrl: $(OBJS)
	$(CC) $(CFLAGS) fan_ctrl.cpp $(OBJS) -o bin/fan_ctrl
rctl_server:
	$(CC) $(CFLAGS) radio_ctrl_tcp.cpp $(OBJS) -o bin/rctl_server

