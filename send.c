#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <json-c/json.h>
#include <hiredis/hiredis.h>

// Globals
redisContext *c;

void redis_conn(void);
void redis_disconn(void);

int main(int argc, char * argv[]) {

  redis_conn();

  redis_disconn();

  return 0;
}

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

void redis_disconn(void) {
  redisFree(c);
}
