#ifndef FILESTORE_INDEX_H
#define FILESTORE_INDEX_H

#include "filestore.h"

ListNode *FileStoreIndexList(TFileStore *FS, const char *Path);

int FileStoreIndexAdd(TFileStore *FS, const char *Name, const char *Path, size_t size, time_t ctime, time_t mtime, time_t expire);


#endif
