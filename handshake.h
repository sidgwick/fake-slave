#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include "client.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int handshake_with_server(server_info *);

#endif
