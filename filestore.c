#include "filestore.h"
#include "fileitem.h"
#include "filestore_dirlist.h"
#include "saved_filestores.h"
#include "settings.h"
#include "errors_and_logging.h"
#include "ui.h"
#include <fnmatch.h>
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
    else RetStr=FileStoreReformatPath(RetStr, DestName, FS);

    return(RetStr);
}



static char *FileStoreAttemptChDir(char *RetPath, TFileStore *FS, const char *DestName)
{
    ListNode *DirList, *Curr;
    TFileItem *FI;

    if (StrValid(DestName)) RetPath=FileStoreGetPath(RetPath, FS, DestName);
    else RetPath=CopyStr(RetPath, FS->HomeDir);
    if (! FS->ChDir) return(RetPath);
    if (FS->ChDir(FS, RetPath)) return(RetPath);

    DirList=FileStoreDirListMatch(FS, NULL, DestName);
    Curr=ListGetNext(DirList);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FI->type == FTYPE_DIR)
        {
            RetPath=FileStoreReformatPath(RetPath, FI->path, FS);
            if (FS->ChDir(FS, RetPath)) return(RetPath);
        }
        Curr=ListGetNext(Curr);
    }

    RetPath=CopyStr(RetPath, "");
    return(RetPath);
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
            HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) CHDIR: $(path)", Path, "");
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
            HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) MKDIR: $(path)", Path, "");
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
            HandleEvent(FS, 0, "$(filestore) DELETE: '$(path)'", FI->path, "");
            result=TRUE;
        }
        else
        {
            HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) DELETE FAILED: '$(path);", FI->path, "");
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

            if (result==TRUE) HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) RMDIR: $(path)", Path, "");
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
    char *Arg1=NULL, *Arg2=NULL;
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
            HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) RENAME: $(path) $(dest)", Path, Arg2);
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
            HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) COPY: $(path) $(dest)", Path, Arg2);
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
        else HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) COPY FAILED: $(path) $(dest)", Path, Arg2);
    }
    else UI_Output(UI_OUTPUT_ERROR, "FileStore does not support file copy");

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
        if (result) HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) CHMOD: $(path) $(dest)", Path, Tempstr);
        else HandleEvent(FS, UI_OUTPUT_DEBUG, "$(filestore) CHMOD FAILED: $(path) $(dest)", Path, Tempstr);
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




void FileStoreGetTimeFromFile(TFileStore *FS)
{
    STREAM *S;
    ListNode *Curr;
    TFileItem *FI;
    char *Tempstr=NULL;
		int StoredFlags;

		//suppress errors while we are doing these 'automatic' tests and transfers
		StoredFlags=Settings->Flags;
		Settings->Flags |= SETTING_NOERROR;
    S=FS->OpenFile(FS, ".fileferry.timetest", "w", 0);
    if (S)
    {
        FS->CloseFile(FS, S);
        //this chdir is done to reload directory listing, so we can
        //access timestamp of .fileferry.timetest
        FileStoreChDir(FS, ".");

        Curr=ListFindNamedItem(FS->DirList, ".fileferry.timetest");
        if (Curr)
        {
            FI=(TFileItem *) Curr->Item;
            FS->TimeOffset=time(NULL) - FI->mtime;
        }

        if (FS->GetValue)
        {
            Tempstr=FS->GetValue(Tempstr, FS,  ".fileferry.timetest", "md5");
            if (StrValid(Tempstr)) SetVar(FS->Vars, "HashTypes", "md5");
            Tempstr=FS->GetValue(Tempstr, FS,  ".fileferry.timetest", "sha");
            if (StrValid(Tempstr)) SetVar(FS->Vars, "HashTypes", "sha");
            Tempstr=FS->GetValue(Tempstr, FS,  ".fileferry.timetest", "sha256");
            if (StrValid(Tempstr)) SetVar(FS->Vars, "HashTypes", "sha256");
        }
    }
		Settings->Flags=StoredFlags;

    Destroy(Tempstr);
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
    float total=0, used=0, avail=0, used_perc=0, avail_perc=0;

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
            Tempstr=MCopyStr(Tempstr, "Disk Space: total: ", ToIEC(total, 2), NULL);
            Value=FormatStr(Value, " used: %0.1f%% (%sb) ", used * 100.0 / total, ToIEC(used, 2));
            Tempstr=CatStr(Tempstr, Value);
            Value=FormatStr(Value, " avail: %0.1f%% (%sb) ", avail * 100.0 / total, ToIEC(avail, 2));
            Tempstr=CatStr(Tempstr, Value);
            UI_Output(0, "%s", Tempstr);
        }
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
}


void FileStoreOutputSupportedFeatures(TFileStore *FS)
{
    const char *ptr;

    if (! FS->Flags & FILESTORE_FOLDERS) UI_Output(0, "This filestore does not support directories/folders");
    if (FS->Flags & FILESTORE_SHARELINK) UI_Output(0, "This filestore supports link sharing via the 'share' command");
    if (FS->Flags & FILESTORE_USAGE) UI_Output(0, "This filestore supports disk-usage/quota reporting via the 'info usage' command");
    if (! FS->ReadBytes) UI_Output(0, "This filestore does NOT support downloads (write only)");
    if (! FS->WriteBytes) UI_Output(0, "This filestore does NOT support uploads (read only)");
    if (! FS->UnlinkPath) UI_Output(0, "This filestore does NOT support deleting files");
    if (! FS->RenamePath) UI_Output(0, "This filestore does NOT support moving/renaming files");
    if (! FS->MkDir) UI_Output(0, "This filestore does NOT support folders/directories");
    ptr=GetVar(FS->Vars, "HashTypes");
    if (StrValid(ptr)) UI_Output(0, "This filestore supports checksum/hashing using %s", ptr);
    if (StrValid(FS->Features)) UI_Output(0, "Protocol Supported Features: %s", FS->Features);
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

                    FileStoreOutputSupportedFeatures(FS);
                    if (StrValid(GetVar(FS->Vars, "ProtocolVersion"))) UI_Output(0, "Protocol Version: %s", GetVar(FS->Vars, "ProtocolVersion"));
                    if (StrValid(GetVar(FS->Vars, "SSL:CipherDetails"))) FileStoreOutputCipherDetails(FS, UI_OUTPUT_VERBOSE);
                    if (StrValid(GetVar(FS->Vars, "ServerBanner"))) UI_Output(0, "%s", GetVar(FS->Vars, "ServerBanner"));

                    if (FS->GetValue) FileStoreOutputDiskQuota(FS);
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

