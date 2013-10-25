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
#include <pthread.h>
#include <curl/curl.h>

#include <json-c/json.h>
#include <hiredis/hiredis.h>

#define NUMTHREADS 8

struct ThreadData {
    int start, stop;
    struct redisReply **array;
};

redisContext *c;
redisReply *keysReply, *tstampReply;
json_object *baseobj;

void* processKeys(void *td);
void processKey(char *);
redisContext* redis_conn(void);
void redis_disconn(redisContext *c);
void handle_reply(redisReply *reply);

int main(int argc, char ** argv) {
  int numKeys, i, start, stop, jobsPerThread;
  struct ThreadData data[NUMTHREADS];
  pthread_t thread[NUMTHREADS];

  c = redis_conn();
  curl_global_init(CURL_GLOBAL_SSL);
  baseobj = json_object_new_object();
  tstampReply = redisCommand(c, "smembers timestamps");
  keysReply = redisCommand(c, "keys *");
  numKeys = (int) keysReply->elements;
  jobsPerThread = (numKeys + NUMTHREADS - 1) / NUMTHREADS;

  fprintf(stdout, "Number of keys %d\nJobs per thread %d\n", numKeys, jobsPerThread);

  for (i = 0; i < NUMTHREADS; i++) {
    data[i].start = i * jobsPerThread;
    data[i].stop = (i + 1) * jobsPerThread;
    data[i].array = keysReply->element;

    if (data[i].stop > numKeys)
      data[i].stop = numKeys;
  }

  // Go through each group and send to function for each key
  for (i = 0; i < NUMTHREADS; i++) {
    // pthread_create(&thread[i], NULL, processKeys, &data[i]);
    processKeys(&data[i]);
  }

  for (i = 0; i < NUMTHREADS; i++) {
    pthread_join(thread[i], NULL);
  }

  redis_disconn(c);
  curl_global_cleanup();
  freeReplyObject(tstampReply);
  freeReplyObject(keysReply);

  FILE *f = fopen("/Users/brian/Desktop/out.json", "w");
  if (f == NULL)
  {
      printf("Error opening file!\n");
      exit(1);
  }

  fprintf(f, "%s\n", json_object_to_json_string(baseobj));

  fclose(f);

  // fprintf(stdout, "%s\n", json_object_to_json_string(baseobj));

  return 0;
}

void* processKeys(void *td) {
  struct ThreadData* data = td;
  int start = data->start;
  int stop = data->stop;
  redisReply **array = data->array;
  int i;
  char *key;

  fprintf(stdout, "Processing %d to %d\n", start, stop);

  for (int i = start; i < stop; i++) {
    key = array[i]->str;
    if (strcmp(key, "timestamps") != 0)
      processKey(key);
  }

  return NULL;
}

void processKey(char *key) {
  fprintf(stdout, "Processing key %s\n", key);

  json_object *powerobj, *r_dbl;
  redisReply *tmpReply;
  char *tstamp, *val;

  powerobj = json_object_new_object();

  for (int j = 0; j < tstampReply->elements; j++) {
    // timestamp
    tstamp = tstampReply->element[j]->str;
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
