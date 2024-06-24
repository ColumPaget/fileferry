#include "filestore.h"
#include "fileitem.h"
#include "filestore_dirlist.h"
#include "saved_filestores.h"
#include "settings.h"
#include "errors_and_logging.h"
#include "ui.h"
#include "filestore_drivers/filestore_drivers.h"



TFileItem *FileStoreGetFileInfo(TFileStore *FS, const char *Path)
{
    TFileItem *FI;
    ListNode *Node;

    Node=ListFindNamedItem(FS->DirList, Path);
    if (Node) return((TFileItem *) Node->Item);
    return(NULL);
}


int FileStoreIsDir(TFileStore *FS, const char *Path)
{
    TFileItem *FI;

    FI=FileStoreGetFileInfo(FS, Path);
    if (! FI) return(FALSE);
    if (FI->type == FTYPE_DIR) return(TRUE);
    return(FALSE);
}


TFileStore *FileStoreCreate()
{
    TFileStore *FS;

    FS=(TFileStore *) calloc(1, sizeof(TFileStore));
    FS->DirList=ListCreate();
    FS->Vars=ListCreate();

    return(FS);
}

void FileStoreDestroy(void *p_FileStore)
{
    TFileStore *FS;

    FS=(TFileStore *) p_FileStore;
    Destroy(FS->URL);
    Destroy(FS->User);
    Destroy(FS->Pass);
    Destroy(FS->Proxy);
    Destroy(FS->CurrDir);
    Destroy(FS->HomeDir);
    Destroy(FS->Handle);
    ListDestroy(FS->Vars, Destroy);
    ListDestroy(FS->DirList, Destroy);
}



char *FileStoreFullURL(char *RetStr, const char *Target, TFileStore *FS)
{
    const char *ptr;

    RetStr=CopyStr(RetStr, FS->URL);
    RetStr=SlashTerminateDirectoryPath(RetStr);

    if (StrValid(FS->CurrDir) && (*Target !='/') && (strcmp(FS->CurrDir, "/") != 0))
    {
        ptr=FS->CurrDir;
        while (*ptr=='/') ptr++;
        RetStr=CatStr(RetStr, ptr);
    }

    if (StrValid(Target))
    {
        RetStr=SlashTerminateDirectoryPath(RetStr);
        ptr=Target;
        while (*ptr=='/') ptr++;
        RetStr=CatStr(RetStr, ptr);
    }

    return(RetStr);
}


const char *FileStorePathRelativeToCurr(TFileStore *FS, const char *Path)
{
    int len;
    const char *ptr;

    if (Path && (*Path=='/'))
    {
        len=StrLen(FS->CurrDir);
        if (strncmp(Path, FS->CurrDir, len)==0)
        {
            ptr=Path+len;
            if (*ptr=='/')
            {
                ptr++;
                return(ptr);
            }

        }
    }

    return(Path);
}


char *FileStoreReformatPath(char *RetStr, const char *Path, TFileStore *FS)
{
    char *Tempstr=NULL;

    if (! StrValid(Path)) return(CopyStr(RetStr, FS->CurrDir));

    if (strncasecmp(Path, "http:", 5)==0) return(CopyStr(RetStr, Path));
    if (strncasecmp(Path, "https:", 6)==0) return(CopyStr(RetStr, Path));
    if (strncasecmp(Path, "gemini:", 7)==0) return(CopyStr(RetStr, Path));

    Tempstr=CopyStr(Tempstr, FS->CurrDir);
    switch (*Path)
    {
    case '/':
        Tempstr=CopyStr(Tempstr, Path);
        break;

    case '.':
        if (strcmp(Path, "..") ==0)
        {
            StrRTruncChar(Tempstr, '/');
            Tempstr=CatStr(Tempstr, "/");
        }
        else if (strcmp(Path, ".")==0) /*do nothing*/ ;
        else
        {
            Tempstr=CopyStr(Tempstr, FS->CurrDir);
            Tempstr=SlashTerminateDirectoryPath(Tempstr);
            Tempstr=CatStr(Tempstr, Path);
        }
        break;

    default:
        Tempstr=CopyStr(Tempstr, FS->CurrDir);
        Tempstr=SlashTerminateDirectoryPath(Tempstr);
        Tempstr=CatStr(Tempstr, Path);
        break;
    }

    //done to handle cases where Tempstr and RetStr are the same string
    RetStr=CopyStr(RetStr, Tempstr);

    Destroy(Tempstr);

    return(RetStr);
}


char *FileStoreReformatDestination(char *RetStr, const char *DestPath, const char *FileName, TFileStore *FS)
{
    ListNode *DirList, *Curr;
    char *FullPath=NULL;
    TFileItem *FI;

    FullPath=FileStoreReformatPath(FullPath, DestPath, FS);
    FI=FileStoreGetFileInfo(FS, GetBasename(FullPath));
    if (FI && (FI->type==FTYPE_DIR))
    {
        RetStr=CopyStr(RetStr,FullPath);
        RetStr=SlashTerminateDirectoryPath(RetStr);
        RetStr=CatStr(RetStr,FileName);
    }
    else RetStr=CopyStr(RetStr,FullPath);

    Destroy(FullPath);

    return(RetStr);
}





char *FileStoreGetPath(char *RetStr, TFileStore *FS, const char *DestName)
{
    ListNode *Node;
    TFileItem *FI;

    Node=ListFindNamedItem(FS->DirList, DestName);
    if (Node)
    {
        FI=(TFileItem *) Node->Item;
        RetStr=FileStoreReformatPath(RetStr, FI->path, FS);
    }
    else
    {
        RetStr=FileStoreReformatPath(RetStr, DestName, FS);
    }

    return(RetStr);
}



static char *FilestoreChDirConsiderItem(char *RetPath, TFileStore *FS, TFileItem *FI)
{
    if (FI)
    {
        if (FI->type == FTYPE_DIR)
        {
            RetPath=FileStoreReformatPath(RetPath, FI->path, FS);
            //if the filestore has no ChDir function (e.g. http/webdav) then it's enough
            //that this path is a directory
            if (! FS->ChDir) return(RetPath);

            //if the filestore does have a ChDir function, then we supply it the path we
            //created in FileStoreReformatPath
            if (FS->ChDir(FS, RetPath)) return(RetPath);
        }
        //if target isn't a directory then return failure
    }

    return(CopyStr(RetPath, ""));
}

static char *FileStoreAttemptChDir(char *RetPath, TFileStore *FS, const char *DestName)
{
    ListNode *DirList, *Curr;
    TFileItem *FI;

    if (StrValid(DestName)) RetPath=FileStoreGetPath(RetPath, FS, DestName);
    else RetPath=CopyStr(RetPath, FS->HomeDir);

    //if ChDir function exists for filestore, and works when we give it the Path
    //then we don't need to do anything clever
    if (FS->ChDir && FS->ChDir(FS, RetPath)) return(RetPath);

    //if we get here, then either:
    //1) The passed in path isn't the path we need to supply to the ChDir function, and we must look that up
    //2) The filestore has no ChDir function, so we must check that the path relates to a directory
    DirList=FileStoreDirListMatch(FS, NULL, DestName);
    Curr=ListGetNext(DirList);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        RetPath=FilestoreChDirConsiderItem(RetPath, FS, FI);
        if (StrValid(RetPath)) return(RetPath);
        Curr=ListGetNext(Curr);
    }



    //handle a couple of special cases that likely *won't* be found, but which
    //we should still consider valid even though they don't exist in file lists
    if (! StrValid(DestName)) return(CopyStr(RetPath, FS->HomeDir));
    if (strcmp(DestName, "..")==0) return(FileStoreReformatPath(RetPath, "..", FS));

    //last chance, if we have fileinfo then we can maybe lookup details of files/directories that are not in the current
    //directory
    if (FS->FileInfo)
    {
        FI=FS->FileInfo(FS, RetPath);
        return(FilestoreChDirConsiderItem(RetPath, FS, FI));
    }


    return(CopyStr(RetPath, ""));
}


int FileStoreChDir(TFileStore *FS, const char *DestName)
{
    int result=FALSE;
    char *Path=NULL;

    if (FS->Flags & FILESTORE_FOLDERS)
    {
        Path=FileStoreAttemptChDir(Path, FS, DestName);
        if (StrValid(Path))
        {
            FS->CurrDir=CopyStr(FS->CurrDir, Path);
            StripDirectorySlash(FS->CurrDir);
            HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) CHDIR: $(path)", Path, "");
            result=TRUE;
            FileStoreDirListClear(FS);
            FileStoreDirListRefresh(FS, 0);
        }
        else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) CHDIR FAILED: $(path)", Path, "");
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support directories/folders");

    Destroy(Path);
    return(result);
}


int FileStoreMkDir(TFileStore *FS, const char *Path, int Mode)
{
    char *Tempstr=NULL;
    int result=FALSE;

    if (FS->MkDir)
    {
        Tempstr=FileStoreReformatPath(Tempstr, Path, FS);
        result=FS->MkDir(FS, Tempstr, Mode);

        if (result==TRUE)
        {
            FileStoreDirListAddItem(FS, FTYPE_DIR, Path, Mode);
            HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) MKDIR: $(path)", Path, "");
        }
        else
        {
            if ((result==ERR_EXISTS) && (Mode & MKDIR_EXISTS_OKAY)) /*do nothing*/;
            else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) MKDIR FAILED: $(path)", Path, "");
        }
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support directories/folders");

    Destroy(Tempstr);

    return(result);
}

int FileStoreUnlinkItem(TFileStore *FS, TFileItem *FI)
{
    char *Tempstr=NULL;
    ListNode *Files, *Curr;
    int result=FALSE;

    if (FS->UnlinkPath)
    {
        Tempstr=FileStoreReformatPath(Tempstr, FI->path, FS);
        if (FS->UnlinkPath(FS, Tempstr)==TRUE)
        {
            FileStoreDirListRemoveItem(FS, FI->name);
            HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) DELETE: '$(path)'", FI->path, "");
            result=TRUE;
        }
        else
        {
            HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) DELETE FAILED: '$(path);", FI->path, "");
        }
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support unlink/delete");

    Destroy(Tempstr);
    return(result);
}


int FileStoreUnlinkPath(TFileStore *FS, const char *Path)
{
    TFileItem *FI;

    FI=FileStoreGetFileInfo(FS, Path);
    if (! FI) return(FALSE);
    return(FileStoreUnlinkItem(FS, FI));
}


int FileStoreRmDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    int result=FALSE;
    TFileItem *FI;

    if (FS->MkDir)
    {
        FI=FileStoreGetFileInfo(FS, Path);
        if (FI && (FI->type == FTYPE_DIR))
        {
            result=FileStoreUnlinkItem(FS, FI);

            if (result==TRUE) HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) RMDIR: $(path)", Path, "");
            else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) RMDIR FAILED: $(path)", Path, "");
        }
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support directories/folders");

    Destroy(Tempstr);

    return(result);
}


int FileStoreLock(TFileStore *FS, const char *Path, int Flags)
{
    if (FS->Lock)
    {
        if (! FS->Lock(FS, Path, Flags))
        {
            UI_Output(UI_OUTPUT_ERROR, "Can't lock %s on filestore %s", Path, FS->URL);
            return(FALSE);
        }
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support locking");

    return(TRUE);
}



int FileStoreUnLock(TFileStore *FS, const char *Path)
{
    if (FS->UnLock)
    {
        if (! FS->UnLock(FS, Path))
        {
            UI_Output(UI_OUTPUT_ERROR, "Can't unlock %s on filestore %s", Path, FS->URL);
            return(FALSE);
        }
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support locking");

    return(TRUE);
}




int FileStoreRename(TFileStore *FS, const char *Path, const char *Dest)
{
    char *Arg1=NULL, *Arg2=NULL, *Tempstr=NULL;
    TFileItem *FI;
    ListNode *Node;
    int result=FALSE, IsMove=FALSE, i;

    if (FS->RenamePath)
    {
        IsMove=FileStoreIsDir(FS, Dest);
        Arg1=FileStoreReformatPath(Arg1, Path, FS);
        Arg2=FileStoreReformatDestination(Arg2, Dest, GetBasename(Path), FS);

        //on some servers trying to rename a file too soon after sending it seems to
        //cause a failure, so we try up to 5 times
        for (i=0; i < 2; i++)
        {
            result=FS->RenamePath(FS, Arg1, Arg2);
            if (result==TRUE) break;
            usleep((i+1) * 500000);
        }

        if (result==TRUE)
        {
            Node=ListFindNamedItem(FS->DirList, GetBasename(Path));
            if (Node)
            {
                if (IsMove) FileStoreDirListRemoveItem(FS, Path);
                else
                {
                    //take a clone so we can delete the existing item
                    FI=FileItemClone((TFileItem *) Node->Item);
                    FileStoreDirListRemoveItem(FS, Path);
                    FI->path=CopyStr(FI->path, Arg2);
                    FI->name=CopyStr(FI->name, GetBasename(FI->path));
                    ListAddNamedItem(FS->DirList, FI->name, FI);
                }
            }
            HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) RENAME: $(path) $(dest)", Path, Arg2);
        }
        else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) RENAME FAILED: $(path) $(dest)", Path, Arg2);
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support file move/rename");

    Destroy(Arg1);
    Destroy(Arg2);

    return(result);
}




int FileStoreCopyFile(TFileStore *FS, const char *Path, const char *Dest)
{
    char *Arg1=NULL, *Arg2=NULL;
    TFileItem *FI;
    ListNode *Node;
    int result=FALSE, i;

    if (FS->CopyPath)
    {
        Arg1=FileStoreReformatPath(Arg1, Path, FS);
        Arg2=FileStoreReformatDestination(Arg2, Dest, GetBasename(Path), FS);

        result=FS->CopyPath(FS, Arg1, Arg2);

        if (result==TRUE)
        {
            HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) COPY: $(path) $(dest)", Path, Arg2);
            Node=ListFindNamedItem(FS->DirList, GetBasename(Path));
            if (Node)
            {
                FI=FileItemClone((TFileItem *) Node->Item);
                FileStoreDirListRemoveItem(FS, Path);
                FI->path=CopyStr(FI->path, Dest);
                FI->name=CopyStr(FI->name, GetBasename(Dest));
                ListAddNamedItem(FS->DirList, FI->name, FI);
            }

        }
        else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) COPY FAILED: $(path) $(dest)", Path, Arg2);
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support file copy");

    Destroy(Arg1);
    Destroy(Arg2);

    return(result);
}


int FileStoreLinkPath(TFileStore *FS, const char *Path, const char *Dest)
{
    char *Arg1=NULL, *Arg2=NULL;
    TFileItem *FI;
    ListNode *Node;
    int result=FALSE, i;

    if (FS->LinkPath)
    {
        Arg1=FileStoreReformatPath(Arg1, Path, FS);
        Arg2=FileStoreReformatDestination(Arg2, Dest, GetBasename(Path), FS);

        result=FS->LinkPath(FS, Arg1, Arg2);

        if (result==TRUE)
        {
            HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) LINK: $(path) $(dest)", Path, Arg2);
            Node=ListFindNamedItem(FS->DirList, GetBasename(Path));
            if (Node)
            {
                FI=FileItemClone((TFileItem *) Node->Item);
                FileStoreDirListRemoveItem(FS, Path);
                FI->path=CopyStr(FI->path, Dest);
                FI->name=CopyStr(FI->name, GetBasename(Dest));
                ListAddNamedItem(FS->DirList, FI->name, FI);
            }

        }
        else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) LINK FAILED: $(path) $(dest)", Path, Arg2);
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support links/symlinks");

    Destroy(Arg1);
    Destroy(Arg2);

    return(result);
}



int FileStoreChMod(TFileStore *FS, const char *ModeStr, const char *Path)
{
    char *Tempstr=NULL;
    unsigned long Mode;
    int result=FALSE;

    if (FS->ChMod)
    {
        Mode=FileSystemParsePermissions(ModeStr);
        Tempstr=FileStoreReformatPath(Tempstr, Path, FS);
        result=FS->ChMod(FS, Tempstr, Mode);
        Tempstr=FormatStr(Tempstr, "%o", Mode);
        if (result) HandleEvent(FS, UI_OUTPUT_VERBOSE, "$(filestore) CHMOD: $(path) $(dest)", Path, Tempstr);
        else HandleEvent(FS, UI_OUTPUT_ERROR, "$(filestore) CHMOD FAILED: $(path) $(dest)", Path, Tempstr);
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support chmod");

    Destroy(Tempstr);

    return(result);
}





char *FileStoreGetValue(char *RetStr, TFileStore *FS, const char *Path, const char *Value)
{
    char *Tempstr=NULL;

    RetStr=CopyStr(RetStr, "");
    if (FS->GetValue)
    {
        Tempstr=FileStoreReformatPath(Tempstr, Path, FS);
        RetStr=FS->GetValue(RetStr, FS, Tempstr, Value);
    }
    Destroy(Tempstr);

    return(RetStr);
}


int FileStoreChPassword(TFileStore *FS, const char *Old, const char *New)
{
    char *Arg1=NULL, *Arg2=NULL;
    TFileItem *FI;
    ListNode *Node;
    int result=FALSE, i;

    if (FS->ChPassword)
    {
        result=FS->ChPassword(FS, Old, New);
        if (! result) UI_Output(UI_OUTPUT_ERROR, "ERROR: failed to change password");
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support changing password");

    Destroy(Arg1);
    Destroy(Arg2);

    return(result);
}




void FileStoreTestFeatures(TFileStore *FS)
{
    STREAM *S;
    TFileItem *FI;
    char *Tempstr=NULL, *FileContents=NULL, *Hash=NULL;
    const char *ptr;
    int StoredFlags, len;

    //suppress errors while we are doing these 'automatic' tests and transfers
    StoredFlags=Settings->Flags;
    Settings->Flags |= SETTING_NOERROR;
    FileContents=GetRandomAlphabetStr(FileContents, 64);
    len=StrLen(FileContents);
    S=FS->OpenFile(FS, ".fileferry.timetest", XFER_FLAG_UPLOAD, 0, len);
    if (S)
    {
        FS->WriteBytes(FS, S, FileContents, 0, len);
        FS->CloseFile(FS, S);

        //this chdir is done to reload directory listing, so we can
        //access timestamp of .fileferry.timetest
        FileStoreChDir(FS, ".");

        FI=FileStoreGetFileInfo(FS, ".fileferry.timetest");
        if (FI) FS->TimeOffset=time(NULL) - FI->mtime;

        if (FS->GetValue)
        {
            ptr=GetVar(FS->Vars, "HashTypes");
            if ( StrValid(ptr) && (strcmp(ptr, "detect")==0) )
            {
                SetVar(FS->Vars, "HashTypes", "");

                Tempstr=FS->GetValue(Tempstr, FS,  ".fileferry.timetest", "md5,sha1,sha256");

                HashBytes(&Hash, "md5", FileContents, StrLen(FileContents), ENCODE_HEX);
                if (strcmp(Tempstr, Hash)==0) AppendVar(FS->Vars, "HashTypes", "md5");

                HashBytes(&Hash, "sha", FileContents, StrLen(FileContents), ENCODE_HEX);
                if (strcmp(Tempstr, Hash)==0) AppendVar(FS->Vars, "HashTypes", "sha");

                HashBytes(&Hash, "sha256", FileContents, StrLen(FileContents), ENCODE_HEX);
                if (strcmp(Tempstr, Hash)==0) AppendVar(FS->Vars, "HashTypes", "sha256");
            }
        }
        if (FI) FileStoreUnlinkItem(FS, FI);
    }
    Settings->Flags=StoredFlags;

    Destroy(FileContents);
    Destroy(Tempstr);
    Destroy(Hash);
}


int FileStoreCompareFileItems(TFileStore *LocalFS, TFileStore *RemoteFS, TFileItem *LocalFI, TFileItem *RemoteFI, char **MatchType)
{
    int RetVal=CMP_MATCH;
    char *LocalHash=NULL, *RemoteHash=NULL, *HashType=NULL;
    const char *ptr;

    if (! LocalFI) return(CMP_NO_LOCAL);
    if (! RemoteFI) return(CMP_NO_REMOTE);
    if (LocalFI->size != RemoteFI->size) return(CMP_SIZE_MISMATCH);

		RetVal=CMP_MATCH;
		if (MatchType) *MatchType=CopyStr(*MatchType, "size");

    ptr=GetVar(RemoteFS->Vars, "HashTypes");
    if (StrValid(ptr))
    {
        GetToken(ptr, " ", &HashType, 0);
        LocalHash=LocalFS->GetValue(LocalHash, LocalFS,  LocalFI->path, HashType);
        RemoteHash=RemoteFS->GetValue(RemoteHash, RemoteFS, RemoteFI->path, HashType);

        if (strcasecmp(LocalHash, RemoteHash)==0)
				{
					if (MatchType) *MatchType=CopyStr(*MatchType, HashType);
				}
        else RetVal=CMP_HASH_MISMATCH;
    }

    if (LocalFI->mtime < RemoteFI->mtime) RetVal=CMP_LOCAL_NEWER;
    if (LocalFI->mtime > RemoteFI->mtime) RetVal=CMP_REMOTE_NEWER;

    Destroy(LocalHash);
    Destroy(RemoteHash);
    Destroy(HashType);

    return(RetVal);
}


int FileStoreCompareFiles(TFileStore *LocalFS, TFileStore *RemoteFS, const char *LocalPath, const char *RemotePath, char **MatchType)
{
    TFileItem *LocalFI, *RemoteFI;

    FileStoreDirListRefresh(LocalFS, DIR_FORCE);
    FileStoreDirListRefresh(RemoteFS, DIR_FORCE);

    LocalFI=FileStoreGetFileInfo(LocalFS, LocalPath);
    RemoteFI=FileStoreGetFileInfo(RemoteFS, RemotePath);

    return(FileStoreCompareFileItems(LocalFS, RemoteFS, LocalFI, RemoteFI, MatchType));
}



TFileStore *FileStoreParse(const char *Config)
{
    char *Proto=NULL, *Tempstr=NULL, *Host=NULL, *Port=NULL, *Path=NULL;
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    TFileStore *FS=NULL;
    STREAM *S;
    int result=FALSE;

    ptr=GetToken(Config, "\\S", &Name, GETTOKEN_QUOTES);
    ptr=GetToken(ptr, "\\S", &Tempstr, 0);
    if (StrValid(Tempstr))
    {
        FS=FileStoreCreate();
        FS->Name=CopyStr(FS->Name, Name);
        ParseURL(Tempstr, &Proto, &Host, &Port, &FS->User, &FS->Pass, &Path, NULL);
        FS->URL=MCopyStr(FS->URL, Proto, "://", Host, NULL);

        if (StrValid(Port))
        {
            if  (strcasecmp(Proto, "https")==0)
            {
                if (atoi(Port) != 443) FS->URL=MCatStr(FS->URL, ":",Port, NULL);
            }
            else FS->URL=MCatStr(FS->URL, ":",Port, NULL);
        }
        if (StrValid(FS->Pass)) CredsStoreAdd(FS->URL, FS->User, FS->Pass);

        //if path doesn't start with a '/' then add one
        if ((! StrValid(Path)) || (*Path != '/')) FS->CurrDir=MCopyStr(FS->CurrDir, "/", Path, NULL);
        else  FS->CurrDir=CopyStr(FS->CurrDir, Path);

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
        while (ptr)
        {
            if (strcasecmp(Name, "user")==0) FS->User=CopyStr(FS->User, Value);
//            else if (strcasecmp(Name, "pass")==0) CredsStoreAdd(FS->URL, FS->User, Value);
//            else if (strcasecmp(Name, "password")==0) CredsStoreAdd(FS->URL, FS->User, Value);
            else if (strcasecmp(Name, "pass")==0) FS->Pass=CopyStr(FS->Pass, Value);
            else if (strcasecmp(Name, "password")==0) FS->Pass=CopyStr(FS->Pass, Value);
            else if (strcasecmp(Name, "encrypt")==0) FS->Encrypt=CopyStr(FS->Encrypt, Value);
            else if (strcasecmp(Name, "credsfile")==0) FS->CredsFile=CopyStr(FS->CredsFile, Value);
            else if (strcasecmp(Name, "proxy")==0) FS->Proxy=CopyStr(FS->Proxy, Value);

            ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
        }
    }

    Destroy(Proto);
    Destroy(Host);
    Destroy(Port);
    Destroy(Path);
    Destroy(Name);
    Destroy(Value);
    Destroy(Tempstr);

    return(FS);
}


//as some filestores will have connections that open and close during use, we record the
//cipher details when a connection is opened
void FileStoreRecordCipherDetails(TFileStore *FS, STREAM *S)
{
    const char *ValuesToCopy[]= {"SSL:Cipher", "SSL:CipherDetails", "SSL:CertificateVerify", "SSL:CertificateCommonName", "SSL:CertificateSubject", "SSL:CertificateIssuer", "SSL:CertificateNotBefore", "SSL:CertificateNotAfter", "SSL:CertificateFingerprint", NULL};
    const char *ptr;
    int i;

    for (i=0; ValuesToCopy[i] !=NULL; i++)
    {
        ptr=STREAMGetValue(S, ValuesToCopy[i]);
        if (StrValid(ptr)) SetVar(FS->Vars, ValuesToCopy[i], ptr);
    }
}


void FileStoreOutputCipherDetails(TFileStore *FS, int Verbosity)
{
    char *Tempstr=NULL;

    Tempstr=CopyStr(Tempstr, GetVar(FS->Vars, "SSL:CipherDetails"));
    StripTrailingWhitespace(Tempstr);
    if (StrValid(Tempstr))
    {
        UI_Output(Verbosity, "Encryption: %s", Tempstr);
        UI_Output(0, "Certificate: %s  Validity: %s  Issuer: %s", GetVar(FS->Vars, "SSL:CertificateCommonName"), GetVar(FS->Vars, "SSL:CertificateVerify"), GetVar(FS->Vars, "SSL:CertificateIssuer"));
        UI_Output(Verbosity, "Certificate Life: %s to %s", GetVar(FS->Vars, "SSL:CertificateNotBefore"), GetVar(FS->Vars, "SSL:CertificateNotAfter"));
        UI_Output(Verbosity, "Certificate Fingerprint: %s", GetVar(FS->Vars, "SSL:CertificateFingerprint"));
    }

    Destroy(Tempstr);
}


void FileStoreOutputDiskQuota(TFileStore *FS)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    double total=0, used=0, avail=0, used_perc=0, avail_perc=0;

    if (FS->GetValue)
    {
        Tempstr=FS->GetValue(Tempstr, FS, "", "DiskQuota");
        if (StrValid(Tempstr))
        {
            ptr=GetNameValuePair(Tempstr, " ", "=", &Name, &Value);
            while (ptr)
            {
                if (strcasecmp(Name, "total")==0) total=atof(Value);
                if (strcasecmp(Name, "avail")==0) avail=atof(Value);
                if (strcasecmp(Name, "used")==0) used=atof(Value);
                ptr=GetNameValuePair(ptr, " ", "=", &Name, &Value);
            }

            //only display disk space if we have some values
            if ((total > 0) || (used > 0) || (avail > 0) ) UI_DisplayDiskSpace(total, used, avail);
        }
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
}



void FileStoreOutputSupportedFeatures(TFileStore *FS)
{
    const char *ptr;

    if (StrValid(GetVar(FS->Vars, "SSL:CipherDetails"))) FileStoreOutputCipherDetails(FS, UI_OUTPUT_VERBOSE);
    if (! FS->Flags & FILESTORE_FOLDERS) UI_Output(0, "This filestore does not support directories/folders");
    if (FS->Flags & FILESTORE_SHARELINK) UI_Output(0, "This filestore supports link sharing via the 'share' command");
    if (FS->Flags & FILESTORE_USAGE) UI_Output(0, "This filestore supports disk-usage/quota reporting via the 'info usage' command");
    if (! FS->ReadBytes) UI_Output(0, "This filestore does NOT support downloads (write only)");
    if (! FS->WriteBytes) UI_Output(0, "This filestore does NOT support uploads (read only)");
    if (FS->Flags & FILESTORE_RESUME_TRANSFERS) UI_Output(0, "This filestore supports resuming transfers via 'get -resume'");
    if (! FS->UnlinkPath) UI_Output(0, "This filestore does NOT support deleting files");
    if (! FS->MkDir) UI_Output(0, "This filestore does NOT support creating folders/directories");
    if (! FS->RenamePath) UI_Output(0, "This filestore does NOT support moving/renaming files");
    if (FS->LinkPath) UI_Output(0, "This filestore supports links/symlinks");
    else UI_Output(0, "This filestore does NOT support links/symlinks");
    ptr=GetVar(FS->Vars, "HashTypes");
    if (StrValid(ptr)) UI_Output(0, "This filestore supports checksum/hashing using %s", ptr);
    if (StrValid(FS->Features)) UI_Output(0, "Protocol Supported Features: %s", FS->Features);
    if (StrValid(GetVar(FS->Vars, "ProtocolVersion"))) UI_Output(0, "Protocol Version: %s", GetVar(FS->Vars, "ProtocolVersion"));
}



TFileStore *FileStoreConnect(const char *Config)
{
    TFileStore *FS=NULL;
    char *Proto=NULL, *Path=NULL, *Name=NULL, *Proxy=NULL, *Tempstr=NULL;
    const char *ptr;

    ptr=GetToken(Config, "\\S", &Name, 0);
    ptr=GetToken(ptr, "\\S", &Name, 0);

    FS=SavedFileStoresFind(Name);
    if (! FS) FS=FileStoreParse(Config);

    if (FS)
    {
        if (StrValid(Settings->ProxyChain)) Proxy=CopyStr(Proxy, Settings->ProxyChain);
        if (StrValid(FS->Proxy)) Proxy=CopyStr(Proxy, FS->Proxy);

        if ( (! StrValid(Proxy)) ||  SetGlobalConnectionChain(Proxy) )
        {
            ParseURL(FS->URL, &Proto, NULL, NULL, NULL, NULL, &Path, NULL);

            if (FileStoreDriverAttach(Proto, FS))
            {
                FS->Flags |= FILESTORE_ATTACHED;
                if (FS->Connect(FS))
                {
                    FS->Flags |= FILESTORE_CONNECTED;
                    if (StrValid(Path)) FileStoreChDir(FS, Path);

                    //don't output any info about local 'file' filestores
                    if (strcasecmp(Proto, "file") !=0)
                    {
                        if (StrValid(GetVar(FS->Vars, "ServerBanner"))) UI_Output(0, "%s", GetVar(FS->Vars, "ServerBanner"));
                        if (StrValid(GetVar(FS->Vars, "LoginBanner"))) UI_Output(0, "%s", GetVar(FS->Vars, "LoginBanner"));

                        FileStoreOutputSupportedFeatures(FS);
                        if (FS->GetValue) FileStoreOutputDiskQuota(FS);
                    }

                    FileStoreDirListRefresh(FS, 0);
                    FS->HomeDir=CopyStr(FS->HomeDir, FS->CurrDir);
                }
            }
        }
        else UI_Output(UI_OUTPUT_ERROR, "Connection Failed to Proxy: %s", Proxy);
    }

    Destroy(Tempstr);
    Destroy(Proto);
    Destroy(Path);
    Destroy(Name);
    Destroy(Proxy);

    return(FS);
}

void FileStoreDisConnect(TFileStore *FS)
{
    if (FS->DisConnect) FS->DisConnect(FS);
}

