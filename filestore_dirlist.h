#ifndef FILEFERRY_FILESTORE_DIRLIST_H
#define FILEFERRY_FILESTORE_DIRLIST_H

#include "filestore.h"

void FileStoreDirListFree(TFileStore *FS, ListNode *Dir);
void FileStoreDirListAddItem(TFileStore *FS, int Type, const char *Name, uint64_t Size);
void FileStoreDirListRemoveItem(TFileStore *FS, const char *Path);
ListNode *FileStoreGetDirList(TFileStore *FS, const char *Path);
void FileStoreDirListClear(TFileStore *FS);
ListNode *FileStoreDirListRefresh(TFileStore *FS, int Flags);
ListNode *FileStoreGlob(TFileStore *FS, const char *Path);
ListNode *FileStoreReloadAndGlob(TFileStore *FS, const char *Glob);
int FileStoreGlobCount(TFileStore *FS, const char *Path);
int FileStoreItemExists(TFileStore *FS, const char *FName, int Flags);


#endif
