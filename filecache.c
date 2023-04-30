#include "filecache.h"
#include "localdisk.h"

TFileStore *FileCacheCreate()
{
    TFileStore *FS;

    char *Tempstr=NULL;

    FS=FileStoreCreate();
    LocalDisk_Attach(FS);
    Tempstr=MCopyStr(Tempstr, getenv("HOME"), "/.filecache", NULL);
    FileStoreMkDir(FS, Tempstr, 0700 | MKDIR_EXISTS_OKAY);
    FileStoreChDir(FS, Tempstr);

    Destroy(Tempstr);

    return(FS);
}
