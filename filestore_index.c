#include "filestore_index.h"
#include "fileitem.h"

int FileStoreIndexAdd(TFileStore *FS, const char *Name, const char *Path, size_t size, time_t ctime, time_t mtime, time_t expire)
{
    char *Tempstr=NULL;
    STREAM *S;

    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.config/fileferry/", FS->Name, ".idx", NULL);
    MakeDirPath(Tempstr, 0700);

    S=STREAMOpen(Tempstr, "a");
    Tempstr=FormatStr(Tempstr, "'%s' 'name=%s' size=%lld ctime=%lld mtime=%lld expire=%lld\n", Path, Name, size, ctime, mtime, expire);
    STREAMWriteLine(Tempstr, S);
    STREAMClose(S);

    Destroy(Tempstr);
}



TFileItem *FileIndexParseEntry(const char *Entry)
{
    TFileItem *FI;
    const char *ptr;
    char *Name=NULL, *Value=NULL;

    FI=FileItemCreate("", 0, 0, 0);
    ptr=GetToken(Entry, "\\S", &FI->path, GETTOKEN_QUOTES);
    ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcmp(Name, "name")==0) FI->name=CopyStr(FI->name, Value);
        else if (strcmp(Name, "size")==0) FI->size=strtoll(Value, NULL, 10);
        else if (strcmp(Name, "mtime")==0) FI->mtime=strtoll(Value, NULL, 10);
        else if (strcmp(Name, "ctime")==0) FI->ctime=strtoll(Value, NULL, 10);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);

    return(FI);
}


ListNode *FileStoreIndexList(TFileStore *FS, const char *Path)
{
    ListNode *DirList;
    char *Tempstr=NULL;
    TFileItem *FI;
    STREAM *S;

    DirList=ListCreate();

    Tempstr=MCopyStr(Tempstr, GetCurrUserHomeDir(), "/.config/fileferry/", FS->Name, ".idx", NULL);
    MakeDirPath(Tempstr, 0700);

    S=STREAMOpen(Tempstr, "r");
    Tempstr=STREAMReadLine(Tempstr, S);
    StripTrailingWhitespace(Tempstr);
    FI=FileIndexParseEntry(Tempstr);
    if (FI) ListAddNamedItem(DirList, FI->name, FI);
    STREAMClose(S);

    Destroy(Tempstr);

    return(DirList);
}
