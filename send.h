/* Header file for send
 * Send is functionality that packages up data from redis in JSON format
 * and sends it to a remote server for persisting and analyzing.
 */

#ifndef SEND
#define SEND

struct ThreadData {
  int start, stop;
  size_t size;
  json_object *baseObj;
  char **keys, **tstamps;
};

#endif
