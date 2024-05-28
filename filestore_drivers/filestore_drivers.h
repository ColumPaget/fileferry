#ifndef FILEFERRY_DRIVERS_H
#define FILEFERRY_DRIVERS_H

#include "../filestore.h"

typedef struct
{
    char *Name;
    char *Description;
    CONNECT_FUNC Attach;
} TFileStoreDriver;

void FileStoreDriversInit();
void FileStoreDriverAdd(const char *Protocol, const char *Name, const char *Description, CONNECT_FUNC Attach);
TFileStoreDriver *FileStoreDriverFind(const char *Protocol);
int FileStoreDriverAttach(const char *Protocol, TFileStore *FS);
ListNode *FileStoreDriversList();

#endif
