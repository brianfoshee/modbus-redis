#ifndef CONNECTION_H
#define CONNECTION_H

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <modbus/modbus.h>
#include <hiredis/hiredis.h>

#define SUNSAVER_ADDR    0x01           // MODBUS address of sunsaver
#ifdef __APPLE__
  #define SERIALPORT      "/dev/tty.usbserial-AD026B5W"
#elif __linux
  #define SERIALPORT      "/dev/ttyUSB0" // or AMA0 if GPIO
#endif

#define REDIS_SERVER    "127.0.0.1"     // Redis server host
#define REDIS_PORT      6379            // Redis server port
#define REDIS_PREFIX    "solar"         // prefix to add in front of keys

// Open and close Redis Connection
redisContext* redis_conn(void);
void redis_disconn(redisContext *c);

// Open and close Modbus connection
modbus_t * modbus_conn(void);
void modbus_disconnect(modbus_t *ctx);

#endif
