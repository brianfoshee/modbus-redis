#include "connection.h"

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

  fprintf(stdout, "Redis connected\n");
  return c;
}

void redis_disconn(redisContext *c) {
  redisFree(c);
  fprintf(stdout, "Redis Disconnected\n");
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
