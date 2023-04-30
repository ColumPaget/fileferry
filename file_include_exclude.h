#ifndef FILE_INCLUDE_EXCLUDE_H
#define FILE_INCLUDE_EXCLUDE_H

#include "filestore.h"
#include "fileitem.h"
#include "commands.h"

int FileIncluded(TCommand *Cmd, TFileItem *FI, TFileStore *FS, TFileStore *ToFS);

#endif

