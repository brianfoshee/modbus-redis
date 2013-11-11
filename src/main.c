#include "send.h"
#include "collect.h"

int main(int argc, char **argv)
{
  // On one thread, start collection
  collectData();
  // On another thread, loop and send every 5 minutes
  sendData();

  return 0;
}
