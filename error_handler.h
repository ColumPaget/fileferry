#ifndef FILEFERRY_ERROR_HANDLER_H
#define FILEFERRY_ERROR_HANDLER_H

#include "common.h"
#include "filestore.h"

void HandleError(TFileStore *FS, int ErrorType, const char *Path, const char *Description);

#endif
