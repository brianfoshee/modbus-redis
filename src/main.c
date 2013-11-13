#include "send.h"
#include "collect.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

static bool running = true;

void mainTermHandler(int);

int main(int argc, char **argv)
{
  signal(SIGINT, mainTermHandler);

  pthread_t thread;
  // On one thread, start collection
  pthread_create(&thread, NULL, collectData, (void *)1);

  // Every 5 minutes send data
  while (running)
  {
    sendData();
  }

  return 0;
}

void mainTermHandler(int d)
{
  running = false;
}
