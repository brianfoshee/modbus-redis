/* Header file for send
 * Send is functionality that packages up data from redis in JSON format
 * and sends it to a remote server for persisting and analyzing.
 */

#ifndef SEND_H
#define SEND_H

#include "connection.h"

void sendData(void);

#endif
