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
#define SERIALPORT      "/dev/ttyUSB0"  // SerialPort address
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

redisContext* redis_conn(void) {
  redisContext *c;
  const char *hostname = REDIS_SERVER;
  int port = REDIS_PORT;

  struct timeval timeout = { 1, 500000 };
  c = redisConnectWithTimeout(hostname, port, timeout);
  if (c == NULL || c->err) {
    if (c) {
      printf("Connection error: %s\n", c->errstr);
      redisFree(c);
    } else {
      printf("Connection error: can't allocate redis context\n");
    }
    exit(-1);
  }
  return c;
}

void redis_disconn(redisContext *c) {
  redisFree(c);
}

modbus_t * modbus_conn(void) {
  modbus_t *ctx;
	ctx = modbus_new_rtu(SERIALPORT, 9600, 'N', 8, 2);
	if (ctx == NULL) {
		fprintf(stderr, "Unable to create the libmodbus context\n");
		exit(-1);
	}

	/* Set the slave id to the SunSaver MPPT MODBUS id */
	modbus_set_slave(ctx, SUNSAVER_ADDR);

	/* Open the MODBUS connection to the SunSaver MPPT */
  if (modbus_connect(ctx) == -1) {
      fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
      modbus_free(ctx);
      exit(-1);
  }
  fprintf(stdout, "Modbus connected\n");
  return ctx;
}

void modbus_disconnect(modbus_t *ctx) {
  modbus_close(ctx);
  modbus_free(ctx);
  fprintf(stdout, "Modbus Disconnected\n");
}

#endif
