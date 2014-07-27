#include "send.h"
#include "collect.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

static bool running = true;

void* sendLoop(void *);
void mainTermHandler(int);

int main(int argc, char **argv)
{
  signal(SIGINT, mainTermHandler);

  pthread_t thread[2];

  pthread_create(&thread[1], NULL, collectData, (void *)1);
  pthread_create(&thread[2], NULL, sendLoop, (void *)1);

  pthread_join(thread[0], NULL);
  pthread_join(thread[1], NULL);

  return 0;
}

//Every 5 minutes send data
void* sendLoop(void *td) {
  while (running)
  {
    sleep(5 * 60);
    sendData();
  }

  return NULL;
}

void mainTermHandler(int d)
{
  running = false;
}
