#ifndef FILEFERRY_WEBDAV_H
#define FILEFERRY_WEBDAV_H

#include "filestore.h"


char *WebDav_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName);
int Webdav_ListDir(TFileStore *FS, const char *URL, ListNode *FileList);

#endif

