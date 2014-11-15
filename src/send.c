/* *****************
 * This program packages up the keys stored in redis as JSON and sends
 * them out to a server to persist, then removes the data it read.
 * Reason is that the RPi has limited memory and storing every 5 seconds
 * uses a lot of memory pretty quickly.
 ***************** */

/* Format of json output
{
  "adc_va_f": {
    "194829148": "12.5"
  }
}
*/

#include "send.h"
#include <pthread.h>

#include <curl/curl.h>
#include <json-c/json.h>

#define PERSIST_URL  "http://192.168.1.119:8080/readings" // URL to send data to

struct ThreadData {
  int start, stop;
  size_t size;
  json_object *baseObj;
  char **keys, **tstamps;
};

void* processKeys(void *td);
void processKey(char *key, char **tstamps, size_t, json_object *, redisContext *);
void handle_reply(redisReply *reply);

void sendData(void) {
  int numKeys, numTstamps, i, skip, start, stop, jobsPerThread;
  char **keys, **tstamps, *key;
  const char *jsonStr;
  redisReply *keysReply, *tstampReply;
  json_object *baseObj;
  redisContext *c;
  FILE *f;
  int j, len;

  c = redis_conn();
  baseObj = json_object_new_object();
  tstampReply = redisCommand(c, "smembers timestamps");
  keysReply = redisCommand(c, "keys solar:*");
  numKeys = (int) keysReply->elements;
  numTstamps = (int) tstampReply->elements;
  keys = malloc(numKeys * sizeof(char*));
  tstamps = malloc(numTstamps * sizeof(char*));

  fprintf(stdout, "Number of keys %d\n", numKeys);
  fprintf(stdout, "Number of tstamps %d\n", numTstamps);
  fflush(stdout);

  // Fill the keys array with all keys
  for (i = 0; i < numKeys; i++)
  {
    key = keysReply->element[i]->str;
    len = strlen(key) - strlen("solar:");
    keys[i] = malloc(len * sizeof(char*));
    strncpy(keys[i], &key[strlen("solar:")], strlen(key));
  }

  // Fill the tstamps array with all timestamps
  for (i = 0; i < numTstamps; i++)
  {
    key = tstampReply->element[i]->str;
    tstamps[i] = malloc(strlen(key) * sizeof(char*));
    strncpy(tstamps[i], key, strlen(key));
  }

  for (i = 0; i < numKeys; i++)
  {
    processKey(keys[i], tstamps, numTstamps, baseObj, c);
    free(keys[i]);
  }

  for (i = 0; i < numTstamps; i++)
  {
    tstampReply = redisCommand(c, "srem timestamps %s", tstamps[i]);
    freeReplyObject(tstampReply);
    free(tstamps[i]);
  }

  // Go ahead and free the redis objects
  freeReplyObject(keysReply);
  free(keys);
  free(tstamps);

  redis_disconn(c);

  jsonStr = json_object_to_json_string(baseObj);
  json_object_put(baseObj);

  f = fopen("/tmp/out.json", "w");

  if (f == NULL)
  {
      printf("Error opening file!\n");
      exit(1);
  }

  fprintf(f, "%s\n", jsonStr);
  fflush(f);

  curl_global_init(CURL_GLOBAL_SSL);

  CURL *handle = curl_easy_init();
  struct curl_slist *headers=NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(handle, CURLOPT_URL, PERSIST_URL);
  curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(handle, CURLOPT_HEADER, 1);

  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, jsonStr);

  CURLcode code = curl_easy_perform(handle);
  curl_slist_free_all(headers);
  curl_global_cleanup();

  fclose(f);
}

void processKey(char *key, char **tstamps, size_t size, json_object *baseObj, redisContext *c)
{
  json_object *powerobj, *r_dbl;
  redisReply *tmpReply;
  char *val, *tstamp;

  fprintf(stdout, "Processing key %s\n", key);
  fflush(stdout);

  powerobj = json_object_new_object();

  // iterate over all the timestamps
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
    tmpReply = redisCommand(c, "HDEL solar:%s %s", key, tstamp);

    freeReplyObject(tmpReply);
  }
  // add the json obj for this key to the base obj
  json_object_object_add(baseObj, key, powerobj);
}
