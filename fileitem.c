#include "fileitem.h"
#include <sys/stat.h>

void FileItemGuessType(TFileItem *FI)
{
    if (S_ISDIR(FI->perms)) FI->type=FTYPE_DIR;
    else if (S_ISLNK(FI->perms)) FI->type=FTYPE_LINK;
    else if (S_ISSOCK(FI->perms)) FI->type=FTYPE_SOCKET;
    else if (S_ISFIFO(FI->perms)) FI->type=FTYPE_FIFO;
    else if (S_ISCHR(FI->perms)) FI->type=FTYPE_CHARDEV;
    else if (S_ISBLK(FI->perms)) FI->type=FTYPE_BLOCKDEV;
}

TFileItem *FileItemCreate(const char *path, int type, uint64_t size, int perms)
{
    TFileItem *FI;

    FI=(TFileItem *) calloc(1, sizeof(TFileItem));
    FI->path=CopyStr(FI->path, path);

    FI->name=CopyStr(FI->name, GetBasename(path));

    //for URL paths clean any HTTP arguments out of the name
    if (
        (strncmp(path, "http:", 5)==0) ||
        (strncmp(path, "https:", 6)==0)
    ) StrRTruncChar(FI->name, '?');


    FI->user=CopyStr(FI->user, "????");
    FI->group=CopyStr(FI->group, "????");
    FI->size=size;
    FI->type=type;
    if (perms==0)
    {
        if (FI->type == FTYPE_DIR) FI->perms=0777;
        else FI->perms=0666;
    }

    return(FI);
}


TFileItem *FileItemClone(TFileItem *Src)
{
    TFileItem *New;

    New=FileItemCreate(Src->path, Src->type, Src->size, Src->perms);
    New->uid=Src->uid;
    New->gid=Src->gid;
    New->atime=Src->atime;
    New->mtime=Src->mtime;
    New->name=CopyStr(New->name, Src->name);
    New->title=CopyStr(New->title, Src->title);
    New->user=CopyStr(New->user, Src->user);
    New->group=CopyStr(New->group, Src->group);
    New->hash=CopyStr(New->hash, Src->hash);
    New->destination=CopyStr(New->destination, Src->destination);
    New->description=CopyStr(New->description, Src->description);
    New->share_url=CopyStr(New->share_url, Src->share_url);
    New->content_type=CopyStr(New->content_type, Src->content_type);

    return(New);
}


void FileItemDestroy(void *p_Item)
{
    TFileItem *Item;

    Item=(TFileItem *) p_Item;
    Destroy(Item->name);
    Destroy(Item->path);
    Destroy(Item->title);
    Destroy(Item->hash);
    Destroy(Item->description);
    Destroy(Item->destination);
    Destroy(Item->share_url);
    Destroy(Item->content_type);
    Destroy(Item->user);
    Destroy(Item->group);
    Destroy(Item);
}


