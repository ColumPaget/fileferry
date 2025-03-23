#ifndef FILEFERRY_EXTRA_HASHES_H
#define FILEFERRY_EXTRA_HASHES_H

#include "common.h"
  

/*
Some filestores implement weird hashing formats. 
Really these should probably be included in the filestore driver
but it's simpler to implement them outside of the driver
to prevent having to lookup the driver when using them
to compare a hash against the local disk, as the local disk driver
doesn't want to be calling another driver
*/

char *DropBoxHashFile(char *RetStr, STREAM *S);


#endif
