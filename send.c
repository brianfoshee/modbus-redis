/* *****************
 * This program packages up the keys stored in redis as JSON and sends
 * them out to a server to persist, then removes the data it read.
 * Reason is that the RPi has limited memory and storing every 5 seconds
 * uses a lot of memory pretty quickly.
 *
 * Compile with:
 * cc -I/usr/local/include -L/usr/local/lib \
      -lcurl -lhiredis -ljson-c send.c -o send
 ***************** */

/* Format of json output
{
  "adc_va_f": {
    "194829148": "12.5"
  }
}
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <curl/curl.h>

#include <json-c/json.h>
#include <hiredis/hiredis.h>

void handle_reply(redisReply *reply);
redisContext* redis_conn(void);
void redis_disconn(redisContext *c);

int main(int argc, char ** argv) {
  int i = 0;
  char *tstamp, *val, *key;
  redisContext *c;
  json_object *baseobj, *powerobj, *r_dbl;
  redisReply *tstamp_reply, *keysReply, *tmpReply;

  c = redis_conn();
  baseobj = json_object_new_object();
  tstamp_reply = redisCommand(c, "smembers timestamps");
  keysReply = redisCommand(c, "keys *");

  // Initialize curl
  curl_global_init(CURL_GLOBAL_SSL);

  for (i = 0; i < keysReply->elements; i++) {
    // ignore key called 'timestamps'
    key = keysReply->element[i]->str;

    if (strcmp(key, "timestamps") != 0) {
      // get all key/vals for this string and store them in a json obj
      powerobj = json_object_new_object();
      for (int j = 0; j < tstamp_reply->elements; j++) {
        // timestamp
        tstamp = tstamp_reply->element[j]->str;
        // get the value for this timestamp
        tmpReply = redisCommand(c, "HGET %s %s", key, tstamp);
        // add key:val pair to powerobj
        val = tmpReply->str;
        if (val != NULL) {
          r_dbl = json_object_new_double(atof(val));
          json_object_object_add(powerobj, tstamp, r_dbl);
        }
        freeReplyObject(tmpReply);
      }
      // add the json obj for this key to the base obj
      json_object_object_add(baseobj, key, powerobj);
    }

  }

  freeReplyObject(tstamp_reply);
  freeReplyObject(keysReply);

  redis_disconn(c);

  curl_global_cleanup();

  printf("%s\n", json_object_to_json_string(baseobj));

  return 0;
}

void handle_reply(redisReply *reply) {
  switch (reply->type) {
    case REDIS_REPLY_STATUS: {
      printf("Received Str %s\n", reply->str);
      break;
    }
    case REDIS_REPLY_ERROR: {
      printf("Received Error %s\n", reply->str);
      break;
    }
    case REDIS_REPLY_INTEGER: {
      printf("Received Integer %lld\n", reply->integer);
      break;
    }
    case REDIS_REPLY_NIL: {
      printf("Received nil reply\n");
      break;
    }
    case REDIS_REPLY_STRING: {
      printf("Received Reply Str %s\n", reply->str);
      break;
    }
    case REDIS_REPLY_ARRAY: {
      printf("Received array of elements of length %ld\n", reply->elements);
      for (int j = 0; j < reply->elements; j++) {
        handle_reply(reply->element[j]);
        // printf("%u) %s\n", j, reply->element[j]->str);
      }
      break;
    }
    default: {
      break;
    }
  }
}

redisContext* redis_conn(void) {
  redisContext *c;
  const char *hostname = "127.0.0.1";
  int port = 6380;

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
