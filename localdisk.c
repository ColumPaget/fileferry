#include "localdisk.h"
#include "fileitem.h"
#include "settings.h"
#include "commands.h"
#include <glob.h>
#include <sys/stat.h>

ListNode *LockFDs=NULL;

TFileItem *LocalDisk_FileInfo(TFileStore *FS, const char *Path)
{
    TFileItem *Item;
    struct stat Stat;
    int Type=FTYPE_FILE;
    int result;

    result=stat(Path, &Stat);
    Item=FileItemCreate(Path, 0, Stat.st_size, Stat.st_mode);
    Item->uid=Stat.st_uid;
    Item->gid=Stat.st_gid;
    Item->atime=Stat.st_atime;
    Item->mtime=Stat.st_mtime;
    FileItemGuessType(Item);

    return(Item);
}

ListNode *LocalDisk_ListDir(TFileStore *FS, const char *Path)
{
    ListNode *FileList;
    char *Tempstr=NULL;
    TFileItem *Item;
    glob_t Glob;
    struct stat Stat;
    int i;

    FileList=ListCreate();
    stat(Path, &Stat);
    if (S_ISDIR(Stat.st_mode)) Tempstr=MCopyStr(Tempstr, Path, "/*", NULL);
    else Tempstr=CopyStr(Tempstr, Path);
    glob(Tempstr, 0, 0, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        Item=LocalDisk_FileInfo(FS, Glob.gl_pathv[i]);
        ListAddNamedItem(FileList, Item->name, Item);
    }
    globfree(&Glob);

    Destroy(Tempstr);
    return(FileList);
}

int LocalDisk_MkDir(TFileStore *FS, const char *Dir, int Mode)
{
    if (mkdir(Dir, Mode)==0) return(TRUE);
    if (errno==EEXIST) return(ERR_EXISTS);
    return(FALSE);
}


int LocalDisk_Unlink(TFileStore *FS, const char *Path)
{
    if (unlink(Path)==0) return(TRUE);
    if (rmdir(Path)==0) return(TRUE);
    return(FALSE);
}


int LocalDisk_Lock(TFileStore *FS, const char *Path, int Flags)
{
    int *fd;
    struct flock Lock;

    fd=(int *) calloc(1, sizeof(int));
    if (Flags & CMD_FLAG_WAIT) *fd=open(Path, O_RDWR | O_CREAT, 0600);
    else *fd=open(Path, O_RDWR | O_CREAT | O_NONBLOCK, 0600);

    Lock.l_type=F_WRLCK;
    Lock.l_whence=SEEK_SET;
    Lock.l_start=0;
    Lock.l_len=0;

    if ( (*fd == -1) || (fcntl(*fd, F_SETLK, &Lock)==-1) )
    {
        free(fd);
        return(FALSE);
    }

    if (! LockFDs) LockFDs=ListCreate();
    ListAddNamedItem(LockFDs, Path, fd);

    return(TRUE);
}


int LocalDisk_UnLock(TFileStore *FS, const char *Path)
{
    int *fd;
    ListNode *Curr;

    Curr=ListFindNamedItem(LockFDs, Path);
    if (Curr)
    {
        fd=(int *) Curr->Item;
        close(*fd);
        free(fd);
        ListDeleteNode(Curr);
        return(TRUE);
    }

    return(FALSE);
}


int LocalDisk_Rename(TFileStore *FS, const char *From, const char *To)
{
    if (rename(From, To)==0) return(TRUE);
    LogToFile(Settings->LogFile, "LOCAL RENAME FAIL: [%s] > [%s] %s", From, To, strerror(errno));
    return(FALSE);
}

int LocalDisk_ChMod(TFileStore *FS, const char *Path, int Mode)
{
    if (chmod(Path, Mode)==0) return(TRUE);
    return(FALSE);
}


char *LocalDisk_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *Name)
{
    RetStr=CopyStr(RetStr, "");

    if (strcasecmp(Name, "crc")==0) HashFile(&RetStr, "crc", Path, ENCODE_HEX);
    if (strcasecmp(Name, "md5")==0) HashFile(&RetStr, "md5", Path, ENCODE_HEX);
    if (strcasecmp(Name, "sha1")==0) HashFile(&RetStr, "sha1", Path, ENCODE_HEX);
    if (strcasecmp(Name, "sha256")==0) HashFile(&RetStr, "sha256", Path, ENCODE_HEX);

    return(RetStr);
}





STREAM *LocalDisk_OpenFile(TFileStore *FS, const char *Path, const char *OpenFlags, uint64_t Size)
{
    STREAM *S;
    char *Tempstr=NULL;

    if (*Path != '/') Tempstr=MCopyStr(Tempstr, FS->CurrDir, "/", Path, NULL);
    else Tempstr=CopyStr(Tempstr, Path);

    S=STREAMOpen(Tempstr, OpenFlags);
    Destroy(Tempstr);

    return(S);
}

int LocalDisk_CloseFile(TFileStore *FS, STREAM *S)
{
    STREAMClose(S);

    return(TRUE);
}

int LocalDisk_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


int LocalDisk_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}



int LocalDisk_Connect(TFileStore *FS)
{
    char *ptr;


    if (strncmp(FS->URL, "file:", 5)==0) ptr=FS->URL+5;
    else ptr=FS->URL;

    while (*ptr=='/') ptr++;

    if ( (! StrValid(ptr)) || (strcmp(ptr, ".")==0) || (strcmp(ptr, "./")==0) ) FS->CurrDir=get_current_dir_name(); //get_current_dir_name mallocs space
    else FS->CurrDir=MCopyStr(FS->CurrDir, "/", ptr, NULL);

    return(TRUE);
}


int LocalDisk_Attach(TFileStore *FS)
{

    FS->Flags |= FILESTORE_FOLDERS;

    FS->Connect=LocalDisk_Connect;
    FS->ListDir=LocalDisk_ListDir;
    FS->FileInfo=LocalDisk_FileInfo;
    FS->MkDir=LocalDisk_MkDir;
    FS->ChMod=LocalDisk_ChMod;
    FS->GetValue=LocalDisk_GetValue;
    FS->UnlinkPath=LocalDisk_Unlink;
    FS->RenamePath=LocalDisk_Rename;
    FS->Lock=LocalDisk_Lock;
    FS->UnLock=LocalDisk_UnLock;
    FS->OpenFile=LocalDisk_OpenFile;
    FS->CloseFile=LocalDisk_CloseFile;
    FS->ReadBytes=LocalDisk_ReadBytes;
    FS->WriteBytes=LocalDisk_WriteBytes;

    return(TRUE);
}
