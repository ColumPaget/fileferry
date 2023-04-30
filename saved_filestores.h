#ifndef FILEFERRY_SAVED_FILESTORES_H
#define FILEFERRY_SAVED_FILESTORES_H

#include "filestore.h"

TFileStore * SavedFilestoresReadEntry(const char *Entry);
ListNode *SavedFilestoresLoad(const char *Path);
int SavedFileStoresSave(const char *Path, ListNode *FileStores);
int SavedFileStoresAdd(TFileStore *FS);
TFileStore *SavedFileStoresFind(const char *Name);
TFileStore *SavedFileStoresList();

#endif
