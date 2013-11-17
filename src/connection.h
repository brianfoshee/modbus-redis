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
#define SERIALPORT      "/dev/ttyAMA0"  // SerialPort address
#define REDIS_SERVER    "127.0.0.1"     // Redis server host
#define REDIS_PORT      6379            // Redis server port
#define REDIS_PREFIX    "solar"         // prefix to add in front of keys
#define PERSIST_URL  "http://192.168.1.8:9292/readings" // URL to send data to

// Open and close Redis Connection
redisContext* redis_conn(void);
void redis_disconn(redisContext *c);

// Open and close Modbus connection
modbus_t * modbus_conn(void);
void modbus_disconnect(modbus_t *ctx);

#endif
