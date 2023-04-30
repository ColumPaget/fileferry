#ifndef FILEFERRY_INET_PROTOCOLS_H
#define FILEFERRY_INET_PROTOCOLS_H

#include "common.h"
#include "filestore.h"

#define INET_CONTINUE 1
#define INET_OKAY 2
#define INET_MORE 4
#define INET_GOOD 7

int InetReadResponse(STREAM *S, TFileStore *FS, char **ResponseCode, char **Verbiage, int RequiredResponse);

#endif
