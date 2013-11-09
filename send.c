/* *****************
 * This program packages up the keys stored in redis as JSON and sends
 * them out to a server to persist, then removes the data it read.
 * Reason is that the RPi has limited memory and storing every 5 seconds
 * uses a lot of memory pretty quickly.
 *
 * Compile with:
 * cc -std=c99 -I/usr/local/include -L/usr/local/lib \
      -lcurl -lhiredis -ljson-c -lpthread send.c -o send
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
  size_t size;
  json_object *baseObj;
  char **keys, **tstamps;
};

void* processKeys(void *td);
void processKey(char *key, char **tstamps, size_t, json_object *);
redisContext* redis_conn(void);
void redis_disconn(redisContext *c);
void handle_reply(redisReply *reply);

int main(int argc, char ** argv) {
  int numKeys, numTstamps, i, skip, start, stop, jobsPerThread;
  char **keys, **tstamps, *key;
  const char *jsonStr;
  redisReply *keysReply, *tstampReply;
  struct ThreadData data[NUMTHREADS];
  pthread_t thread[NUMTHREADS];
  json_object *baseObj;
  redisContext *c;
  FILE *f;

  c = redis_conn();
  baseObj = json_object_new_object();
  tstampReply = redisCommand(c, "smembers timestamps");
  keysReply = redisCommand(c, "keys solar:*");
  numKeys = (int) keysReply->elements;
  numTstamps = (int) tstampReply->elements;
  jobsPerThread = (numKeys + NUMTHREADS - 1) / NUMTHREADS;
  keys = malloc(numKeys * sizeof(char*));
  tstamps = malloc(numTstamps * sizeof(char*));

  fprintf(stdout, "Number of keys %d\n", numKeys);
  fprintf(stdout, "Number of tstamps %d\n", numTstamps);
  fprintf(stdout, "Jobs per thread %d\n", jobsPerThread);

  // Fill the keys array with all keys
  for (i = 0; i < numKeys; i++)
  {
    key = keysReply->element[i]->str;
    keys[i] = malloc(strlen(key) * sizeof(char*));
    strncpy(keys[i], key, strlen(key));
  }

  // Fill the tstamps array with all timestamps
  for (i = 0; i < numTstamps; i++)
  {
    key = tstampReply->element[i]->str;
    tstamps[i] = malloc(strlen(key) * sizeof(char*));
    strncpy(tstamps[i], key, strlen(key));
  }

  // Go ahead and free the redis objects
  freeReplyObject(tstampReply);
  freeReplyObject(keysReply);

  for (i = 0; i < NUMTHREADS; i++)
  {
    data[i].start = i * jobsPerThread;
    data[i].stop = (i + 1) * jobsPerThread;
    data[i].size = numTstamps;
    data[i].keys = keys;
    data[i].tstamps = tstamps;
    data[i].baseObj = baseObj;

    if (data[i].stop > numKeys)
      data[i].stop = numKeys;
  }

  for (i = 0; i < NUMTHREADS; i++)
  {
    pthread_create(&thread[i], NULL, processKeys, &data[i]);
  }

  for (i = 0; i < NUMTHREADS; i++)
  {
    pthread_join(thread[i], NULL);
  }

  for (i = 0; i < numKeys; i++)
  {
    free(keys[i]);
  }

  for (i = 0; i < numTstamps; i++)
  {
    free(tstamps[i]);
  }

  free(keys);
  free(tstamps);

  redis_disconn(c);

  jsonStr = json_object_to_json_string(baseObj);

  f = fopen("/tmp/out.json", "w");

  if (f == NULL)
  {
      printf("Error opening file!\n");
      exit(1);
  }

  fprintf(f, "%s\n", jsonStr);

  curl_global_init(CURL_GLOBAL_SSL);

  CURL *handle = curl_easy_init();
  struct curl_slist *headers=NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(handle, CURLOPT_URL, "http://192.168.1.8:9292/readings");
  curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(handle, CURLOPT_HEADER, 1);

  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, jsonStr);

  CURLcode code = curl_easy_perform(handle);
  curl_slist_free_all(headers);
  curl_global_cleanup();

  fclose(f);

  return 0;
}

void* processKeys(void *td)
{
  struct ThreadData* data = td;
  int start = data->start;
  int stop = data->stop;
  size_t size = data->size;
  char **keys = data->keys;
  char **tstamps = data->tstamps;
  json_object *baseObj = data->baseObj;
  int i, j, len;
  char *key, *tmp;

  fprintf(stdout, "Processing %d to %d\n", start, stop);

  /* TODO do some substring substitution to pull 'solar:' out of the key */
  for (i = start; i < stop; i++) {
    tmp = keys[i];
    len = strlen(tmp) - strlen("solar:");
    memcpy(&key, tmp + 6, len);
    fprintf(stdout, "Key is %s", key);

    processKey(key, tstamps, size, baseObj);
  }

  return NULL;
}

void processKey(char *key, char **tstamps, size_t size, json_object *baseObj)
{
  json_object *powerobj, *r_dbl;
  redisReply *tmpReply;
  char *val, *tstamp;
  redisContext *c;

  fprintf(stdout, "Processing key %s\n", key);

  c = redis_conn();
  powerobj = json_object_new_object();

  for (int j = 0; j < (int) size; j++) {
    tstamp = tstamps[j];
    // get the value for this timestamp
    tmpReply = redisCommand(c, "HGET solar:%s %s", key, tstamp);
    // add key:val pair to powerobj
    val = tmpReply->str;
    if (val != NULL) {
      r_dbl = json_object_new_double(atof(val));
      json_object_object_add(powerobj, tstamp, r_dbl);
    }
    freeReplyObject(tmpReply);
  }
  // add the json obj for this key to the base obj
  json_object_object_add(baseObj, key, powerobj);
  redis_disconn(c);
}

redisContext* redis_conn(void) {
  redisContext *c;
  const char *hostname = "127.0.0.1";
  int port = 6379;

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
