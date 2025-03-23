#ifndef FILEFERRY_FILESTORE_DIRLIST_H
#define FILEFERRY_FILESTORE_DIRLIST_H

#include "filestore.h"
#include "fileitem.h"

void FileStoreDirListFree(TFileStore *FS, ListNode *Dir);
void FileStoreDirListRemoveItem(TFileStore *FS, const char *Path);
ListNode *FileStoreGetDirList(TFileStore *FS, const char *Path);
void FileStoreDirListClear(TFileStore *FS);
ListNode *FileStoreDirListRefresh(TFileStore *FS, int Flags);
ListNode *FileStoreGlob(TFileStore *FS, const char *Path);
ListNode *FileStoreReloadAndGlob(TFileStore *FS, const char *Glob);
int FileStoreGlobCount(TFileStore *FS, const char *Path);
TFileItem *FileStoreItemExists(TFileStore *FS, const char *FName, int Flags);
TFileItem *FileStoreDirListUpdateItem(TFileStore *FS, int Type, const char *Name, uint64_t Size, int Perms);
ListNode *FileStoreDirListMatch(TFileStore *FS, ListNode *InputList, const char *Match);

#endif
