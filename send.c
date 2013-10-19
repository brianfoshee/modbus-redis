#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <json-c/json.h>
#include <hiredis/hiredis.h>

// Globals
redisContext *c;

void handle_reply(redisReply *reply);
void redis_conn(void);
void redis_disconn(void);

int main(int argc, char ** argv) {
  if (argc < 2) {
    printf("Usage: ./send KEY\n");
    exit(0);
  }

  char *key = argv[1];

  redis_conn();

  /*
  {
    "adc_va_f": {
      194829148: 12.5
    }
  }
  */

  json_object *baseobj = json_object_new_object();
  json_object *powerobj = json_object_new_object();
  json_object *r_str;
  char *tstamp, *val;

  redisReply *reply;
  redisReply *r;

  reply = redisCommand(c, "smembers timestamps");

  for (int j = 0; j < reply->elements; j++) {
    // timestamp
    tstamp = reply->element[j]->str;
    // get the value for this timestamp
    r = redisCommand(c, "HGET %s %s", key, tstamp);
    // add key:val pair to powerobj
    val = r->str;
    if (val != NULL) {
      r_str = json_object_new_string(val);
      json_object_object_add(powerobj, tstamp, r_str);
    }
    freeReplyObject(r);
  }

  freeReplyObject(reply);

  json_object_object_add(baseobj, key, powerobj);

  redis_disconn();

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

void redis_conn(void) {
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
}

void redis_disconn(void) {
  redisFree(c);
}
