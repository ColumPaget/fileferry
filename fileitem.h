
#ifndef FILEFERRY_FILEITEM_H
#define FILEFERRY_FILEITEM_H

#include "common.h"

typedef enum {FTYPE_FILE, FTYPE_DIR, FTYPE_LINK, FTYPE_CHARDEV, FTYPE_BLOCKDEV, FTYPE_FIFO, FTYPE_SOCKET} EFileTypes;

typedef struct
{
    char *name;
    char *path;
    char *share_url;
    char *destination; //for symlinks, path to real file
    char *title;
    char *description;
    char *content_type;
    char *user;
    char *group;
    char *hash;
    int type;
    int uid;
    int gid;
    int perms;
    uint64_t size;
    uint64_t ctime;
    uint64_t atime;
    uint64_t mtime;
} TFileItem;

void FileItemGuessType(TFileItem *FI);
TFileItem *FileItemCreate(const char *name, int type, uint64_t size, int perms);
TFileItem *FileItemClone(TFileItem *Src);
void FileItemDestroy(void *Item);

#endif
