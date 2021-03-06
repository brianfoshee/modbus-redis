/*
 *  Reads in data from a Sunsaver MPPT charge controller, stores to Redis.
 */

#include "collect.h"
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

// Global Vars
static bool running = true;

// Method Definitions
uint16_t * read_data(modbus_t *ctx);
void setData(char *key, float val, int t, redisContext *c);
void handleData(uint16_t *, int t, redisContext *c);
void termHandler(int dum);

void collectData(void)
{
  signal(SIGINT, termHandler);

  uint16_t *data;
  redisContext *r_ctx;
  modbus_t *m_ctx;
  int t;

  r_ctx = redis_conn();

  m_ctx = modbus_conn();

  while(running) {
    t = (int)time(NULL);

    data = read_data(m_ctx);

    handleData(data, t, r_ctx);

    free(data);

    sleep(5);
  }

  modbus_disconnect(m_ctx);

  redis_disconn(r_ctx);
}

/* WARNING: caller is responsible for free()ing return value! */
uint16_t * read_data(modbus_t *ctx) {
	/* Read the RAM Registers */
  int rc;
  uint16_t *data = malloc(sizeof(uint16_t) * 44);
  // Read in the first half of registers
	rc = modbus_read_registers(ctx, 0x08, 22, data);
	if (rc == -1) {
		fprintf(stderr, "Modbus Error (first half): %s\n", modbus_strerror(errno));
		exit(-1);
	}
  sleep(1);
  // Read in the second half of registers
	rc = modbus_read_registers(ctx, 0x1E, 22, data+22);
	if (rc == -1) {
		fprintf(stderr, "Modbus Error (second half):%s\n", modbus_strerror(errno));
		exit(-1);
	}
  return data;
}

void setData(char *key, float val, int t, redisContext *c) {
  redisReply *reply;

  reply = redisCommand(c, "HSET solar:%s %d %f", key, t, val);

  freeReplyObject(reply);
}

void handleData(uint16_t *data, int t, redisContext *c) {
  // Set a master set of timestamps. Makes it easier to access
  // things in HGET
  redisReply *reply;
  reply = redisCommand(c, "SADD timestamps %d", t);
  freeReplyObject(reply);
  float sb = 65536.0;
  float fb = 32768.0;
  float conv = 100.0 / fb;
  float sconv = 989.5 / sb;

  //fprintf(stdout, "Handling Data at %d\n", t);
  //fflush(stdout);

  // Voltage measured directly at the battery terminal
  setData("Adc_vb_f", data[0] * conv, t, c);

  // Terminal voltage of solar
  setData("Adc_va_f", data[1] * conv, t, c);

  // Terminal voltage of load
  setData("Adc_vl_f", data[2] * conv, t, c);

  // Charging current to the battery
  setData("Adc_ic_f", data[3] * 79.16 / fb, t, c);

  // Load current to load
  setData("Adc_il_f", data[4] * 79.16 / fb, t, c);

  // Heatsink temp
  setData("T_hs", data[5], t, c);

  // Battery temp
  setData("T_batt", data[6], t, c);

  // Ambient temp (builtin)
  setData("T_amb", data[7], t, c);

  // Remote battery temp sensor (0x08 if not connected)
  setData("T_rts", data[8], t, c);

  // Charge state (nums in ref)
  setData("charge_state", data[9], t, c);

  // Array fault bitfield
  setData("array_fault", data[10], t, c);

  // Battery Voltage (slow filtered)
  setData("Vb_f",data[11] * conv, t, c);

  // Target voltage to which battery will be charged (temp compensated)
  setData("Vb_ref", data[12] * 96.667 / fb, t, c);

  // Solar amp-hours since last reset
  // Reset-able
  setData("Ahc_r", ((data[13] << 16) + data[14]) * 0.1, t, c);

  // Solar amp-hours since last reset
  // cumulative
  setData("Ahc_t",((data[15] << 16) + data[16]) * 0.1, t, c);

  // Total solar kWh since last reset
  // reset-able
  setData("kWhc",data[17] * 0.1, t, c);

  // Load state
  setData("load_state", data[18], t, c);

  // Load fault
  setData("load_fault", data[19], t, c);

  // Low voltage disconnect setpoint, current compensated
  setData("V_lvd", data[20] * conv, t, c);

  // Load amp-hours since last reset
  // Resetable
  setData("Ahl_r", ((data[21] << 16) + data[22]) * 0.1, t, c);

  // Load amp hours
  // Total cumulative
  setData("Ahl_t", ((data[23] << 16) + data[24]) * 0.1, t, c);

  // Total hours of operation since installed
  setData("hourmeter", (data[25] << 16) + data[26], t, c);

  // Alarm
  setData("alarm", (data[27] << 16) + (data[28] << 16), t, c);

  // DIP switch
  setData("dip_switch", data[29], t, c);

  // LED State
  setData("led_state", data[30], t, c);

  // Output power to the battery
  setData("Power_out", data[31] * sconv, t, c);

  // Max power voltage of the solar array during last sweep
  setData("Sweep_Vmp", data[32]* conv, t, c);

  // max power output of solar array during last sweep
  setData("Sweep_Pmax",data[33] * sconv, t, c);

  // Open circuit voltage of solar array during last sweep
  setData("Sweep_Voc", data[34] * conv, t, c);

  // Minimum battery voltage (resets after dark)
  setData("Vb_min_daily",data[35] * conv, t, c);

  // Maximum battery voltage (resets after dark)
  setData("Vb_max_daily", data[36] * conv, t, c);

  // Total charging amp-hours today (resets after dark)
  setData("Ahc_daily", data[37] * 0.1, t, c);

  // Total load amp hours today (resets after dark)
  setData("Ahl_daily", data[38] * 0.1, t, c);

  // Array fault daily
  setData("array_fault_daily", data[39], t, c);

  // Load fault daily
  setData("load_fault_daily", data[40], t, c);

  // alarm daily
  setData("alarm_daily", (data[41] << 16) + (data[42] << 16), t, c);

  // min battery voltage
  setData("vb_min", data[43] * conv, t, c);

  // Max battery voltage
  setData("vb_max", data[44] * conv, t, c);
}

void termHandler(int dum) {
  running = false;
}
