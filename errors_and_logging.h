#ifndef FILEFERRY_ERROR_HANDLER_H
#define FILEFERRY_ERROR_HANDLER_H

#include "common.h"
#include "filestore.h"
#include "ui.h"

void LogInit();
void HandleEvent(TFileStore *FS, int ErrorType, const char *Description, const char *Path, const char *Dest);

int SendLoggedLine(const char *Line, TFileStore *FS, STREAM *S);
char *ReadLoggedLine(char *Line, TFileStore *FS, STREAM *S);


#endif
