/*
 *  Reads in data from a Sunsaver MPPT charge controller, stores to Redis.
 *  Compile with: cc -I/usr/local/include/modbus -I/usr/local/include/hiredis \
                     -L/usr/local/lib -lmodbus -lhiredis  sunsaver.c -o sunsaver
 *
 *  vim /etc/ld.so.conf.d/libc.conf
 * <- '/usr/local/lib'
 * ldconfig
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <modbus.h>
#include "hiredis.h"

#define SUNSAVERMPPT    0x01	/* MODBUS Address of the SunSaver MPPT */
#define SERIALPORT      "/dev/ttyUSB0"

static bool running = true;

void termHandler(int dummy) {
  running = false;
}

// Globals
redisContext *c;
modbus_t *ctx;

void redis_conn(void) {
  const char *hostname = "127.0.0.1";
  int port = 6379;

  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
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
}

void modbus_conn(void) {
	/* Set up a new MODBUS context */
	ctx = modbus_new_rtu(SERIALPORT, 9600, 'N', 8, 2);	/* Add the appropriate path to your serial port */
	if (ctx == NULL) {
		fprintf(stderr, "Unable to create the libmodbus context\n");
		exit(-1);
	}

	/* Set the slave id to the SunSaver MPPT MODBUS id */
	modbus_set_slave(ctx, SUNSAVERMPPT);

	/* Open the MODBUS connection to the SunSaver MPPT */
  if (modbus_connect(ctx) == -1) {
      fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
      modbus_free(ctx);
      exit(-1);
  }
  fprintf(stdout, "Modbus connected\n");
}

void modbus_disconnect(void) {
	/* Close the MODBUS connection */
  modbus_close(ctx);
  modbus_free(ctx);
  fprintf(stdout, "Modbus Disconnected\n");
}

/* WARNING: caller is responsible for free()ing return value! */
uint16_t * read_data() {
	/* Read the RAM Registers */
  int rc;
  uint16_t *data = malloc(sizeof(uint16_t) * 50);
	rc = modbus_read_registers(ctx, 0x0008, 45, data);
	if (rc == -1) {
		fprintf(stderr, "%s\n", modbus_strerror(errno));
		exit(-1);
	}
  return data;
}

/* WARNING: caller is responsible for free()ing return value! */
char * numToStr(float num) {
  char *buf = malloc(10);
  sprintf(buf, "%.2f", num);
  return buf;
}

void setData(char *key, float val) {
  redisReply *reply;
  int t = (int)time(NULL);
  char *num = numToStr(val);

  reply = redisCommand(c ,"SET %s:%d %s",key, t, num);
  printf("SET %s:%d to %s %s\n",key, t, num, reply->str);

  freeReplyObject(reply);
  free(num);
}

void handleData(uint16_t *);

int main()
{
  signal(SIGINT, termHandler);

  uint16_t *data;

  redis_conn();

  modbus_conn();

  while(running) {
    data = read_data();

    handleData(data);

    sleep(5);
  }

  modbus_disconnect();

  redisFree(c);

	return 0;
}

uint16_t * uintdup(uint16_t const * src, size_t len)
{
   uint16_t * p = malloc(len * sizeof(uint16_t));
   memcpy(p, src, len * sizeof(uint16_t));
   return p;
}

void handleData(uint16_t *data) {

  // Voltage measured directly at the battery terminal
  setData("adc_vb_f", data[0] * 100.0 / 32768.0);

  // Terminal voltage of solar
  setData("adc_va_f", data[1] * 100.0 / 32768.0);

  // Terminal voltage of load
  setData("adc_vl_f", data[2] * 100.0 / 32768.0);

  // Charging current to the battery
	setData("adc_ic_f", data[3] * 79.16 / 32768.0);

  // Load current to load
	setData("adc_il_f", data[4] * 79.16 / 32768.0);

  // Heatsink temp
	setData("T_hs", data[5]);

  // Ambient temp (builtin)
	setData("T_amb", data[7]);

  // Charge state (nums in ref)
	setData("charge_state", data[9]);

  // Battery Voltage (slow filtered)
	setData("Vb_f",data[11]*100.0/32768.0);

  // Target voltage to which battery will be charged (temp compensated)
	setData("Vb_ref", data[12]*96.667/32768.0);

  // Solar amp-hours since last reset
  // Reset-able
	setData("Ahc_r", ((data[13] << 16) + data[14])*0.1);

  // Solar amp-hours since last reset
  // cumulative
	setData("Ahc_t",((data[15] << 16) + data[16])*0.1);

  // Total solar kWh since last reset
  // reset-able
	setData("kWhc",data[17]*0.1);

  // Low voltage disconnect setpoint, current compensated
	setData("V_lvd", data[20]*100.0/32768.0);

  // Load amp-hours since last reset
  // Resetable
	setData("Ahl_r", ((data[21] << 16) + data[22])*0.1);

  // Load amp hours
  // Total cumulative
	setData("Ahl_t", ((data[23] << 16) + data[24])*0.1);

  // Total hours of operation since installed
	setData("hourmeter", (data[25] << 16) + data[26]);

  // Output power to the battery
	setData("Power_out", data[31]*989.5/65536.0);

  // Max power voltage of the solar array during last sweep
	setData("Sweep_Vmp", data[32]*100.0/32768.0);

  // max power output of solar array during last sweep
	setData("Sweep_Pmax",data[33]*989.5/65536.0);

  // Open circuit voltage of solar array during last sweep
	setData("Sweep_Voc", data[34]*100.0/32768.0);

  // Minimum battery voltage (resets after dark)
	setData("Vb_min_daily",data[35]*100.0/32768.0);

  // Maximum battery voltage (resets after dark)
	setData("Vb_max_daily", data[36]*100.0/32768.0);

  // Total charging amp-hours today (resets after dark)
	setData("Ahc_daily", data[37]*0.1);

  // Total load amp hours today (resets after dark)
	setData("Ahl_daily", data[38]*0.1);

  free(data);
}
