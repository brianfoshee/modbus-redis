/* Header file for send
 * Send is functionality that packages up data from redis in JSON format
 * and sends it to a remote server for persisting and analyzing.
 */

#ifndef SEND
#define SEND

#include <hiredis/hiredis.h>

#define REDIS_SERVER    "127.0.0.1"    // Redis server host
#define REDIS_PORT      6379           // Redis server port

redisContext* redis_conn(void);
void redis_disconn(redisContext *c);

#endif
