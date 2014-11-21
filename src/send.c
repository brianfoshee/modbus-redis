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

/*
 * Should change to:
 * {
 *   "194829148": {
 *     "adc_va_f": 12.5
 *   }
 * }
 *
 * */

#include "send.h"

#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define PERSIST_URL  "http://10.10.10.164:8080/readings" // URL to send data to

void* processKeys(void *td);
void processKey(char *key, char **tstamps, size_t, json_object *, redisContext *);
void handle_reply(redisReply *reply);

void sendData(void) {
  long numTstamps, numKeys, i;
  char **keys, **tstamps, *key;
  const char *jsonStr;
  redisReply *reply;
  json_object *baseObj;
  redisContext *c;
  unsigned long len;

  c = redis_conn();
  baseObj = json_object_new_object();

  // Fill the keys array with all keys
  reply = redisCommand(c, "keys solar:*");
  numKeys = (long) reply->elements;
  keys = malloc(numKeys * sizeof(char*));
  fprintf(stdout, "Number of keys %lu\n", numKeys);
  fflush(stdout);
  for (i = 0; i < numKeys; i++) {
    key = reply->element[i]->str;
    len = strlen(key) - strlen("solar:");
    keys[i] = malloc(len * sizeof(char*));
    strncpy(keys[i], &key[strlen("solar:")], strlen(key));
  }
  freeReplyObject(reply);
  reply = NULL;

  // Fill the tstamps array with all timestamps
  reply = redisCommand(c, "smembers timestamps");
  numTstamps = (long) reply->elements;
  tstamps = malloc(numTstamps * sizeof(char*));
  fprintf(stdout, "Number of tstamps %lu\n", numTstamps);
  fflush(stdout);
  for (i = 0; i < numTstamps; i++) {
    key = reply->element[i]->str;
    tstamps[i] = malloc(strlen(key) * sizeof(char*));
    strncpy(tstamps[i], key, strlen(key));
  }
  freeReplyObject(reply);
  reply = NULL;

  for (i = 0; i < numKeys; i++) {
    processKey(keys[i], tstamps, numTstamps, baseObj, c);
    free(keys[i]);
    keys[i] = NULL;
  }
  free(keys);
  keys = NULL;

  for (i = 0; i < numTstamps; i++) {
    reply = redisCommand(c, "srem timestamps %s", tstamps[i]);
    freeReplyObject(reply);
    free(tstamps[i]);
    tstamps[i] = NULL;
  }
  free(tstamps);
  tstamps = NULL;

  redis_disconn(c);
  c = NULL;

  jsonStr = json_object_to_json_string_ext(baseObj, JSON_C_TO_STRING_PLAIN);

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
  /* Check for errors */
  if(code != CURLE_OK) {
    fprintf(stdout, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(code));
  }
  curl_slist_free_all(headers);
  curl_easy_cleanup(handle);
  curl_global_cleanup();

  json_object_put(baseObj);
  baseObj = NULL;
}

void processKey(char *key, char **tstamps, size_t size, json_object *baseObj, redisContext *c) {
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
