#include "filebin.h"
#include "../ui.h"

static char *FileBin_BinName(char *RetStr, TFileStore *FS)
{
    char *Tempstr=NULL;
    const char *ptr;

    ptr=FS->URL;
    if (StrLen(ptr) > 8) ptr += 8;
    while (*ptr=='/') ptr++;

    switch (*ptr)
    {
    case '~':
        RetStr=CopyStr(RetStr, ptr+1);
        break;

    case '\0':
        Tempstr=FormatStr(Tempstr, "%d:%ld:%ld", getpid(), time(NULL), rand());
        ptr=Tempstr;
    //fall through

    default:
        HashBytes(&RetStr, "sha1", ptr, StrLen(ptr), ENCODE_HEX);
        StrTrunc(RetStr, 16);
        break;
    }

    Destroy(Tempstr);
    return(RetStr);
}


static TFileItem *FileBin_GetItem(ListNode *Items, const char *Name, int Type, uint64_t size)
{
    ListNode *FINode;
    TFileItem *FI=NULL;

    FINode=ListFindNamedItem(Items, Name);
    if (FINode) FI=(TFileItem *) FINode->Item;
    else
    {
        FI=FileItemCreate(Name, Type, size, 0666);
        ListAddNamedItem(Items, FI->name, FI);
    }

    return(FI);
}


static void FileBin_ParseItem(ListNode *Items, const char *JSON)
{
    ListNode *Curr, *FINode;
    TFileItem *FI;
    PARSER *P;
    STREAM *S;
    uint64_t size;
    char *FName=NULL;

    P=ParserParseDocument("json", JSON);
    Curr=ParserOpenItem(P, "/files");
    // maybe there's just a single file?
    if (! Curr) Curr=ParserOpenItem(P, "/file");
    while (Curr)
    {
        FName=CopyStr(FName, ParserGetValue(Curr, "filename"));
        if (StrValid(FName))
        {
            size=(uint64_t) strtoll(ParserGetValue(Curr, "bytes"), NULL, 10);
            FI=FileBin_GetItem(Items, FName, FTYPE_FILE, size);
            FI->ctime=DateStrToSecs("%Y-%m-%dT%H:%M:%S", ParserGetValue(Curr, "created_at"), NULL);
            FI->mtime=DateStrToSecs("%Y-%m-%dT%H:%M:%S", ParserGetValue(Curr, "updated_at"), NULL);
            //md5 is stored using an unknown scheme that doesn't match md5sum, so we ignore it
            //FI->hash=MCopyStr(FI->hash, "md5:", ParserGetValue(Curr, "md5"), " ", "sha256:", ParserGetValue(Curr, "sha256"), NULL);
            //sha256 matches sha256sum
            FI->hash=MCopyStr(FI->hash, "sha256:", ParserGetValue(Curr, "sha256"), NULL);
            FI->share_url=MCopyStr(FI->share_url, "https://filebin.net/", ParserGetValue(P, "/bin/id"), "/", FI->name, NULL);
        }
        Curr=ListGetNext(Curr);
    }

    ParserItemsDestroy(P);
    Destroy(FName);
}


static ListNode *FileBin_ListDir(TFileStore *FS, const char *Path)
{
    ListNode *Items;
    char *URL=NULL, *Tempstr=NULL;
    STREAM *S;

    Items=ListCreate();
    if ( (strcmp(Path, "/")==0) || (strcmp(Path, ".")==0) || (strcmp(Path, FS->CurrDir)==0) )
    {
        FileBin_GetItem(Items, ".", FTYPE_DIR, 0);
    }
    else
    {
        URL=MCopyStr(URL, "https://filebin.net/", FS->CurrDir, NULL);
        S=STREAMOpen(URL, "r Accept=application/json");
        if (S)
        {
            FS->Flags |= FILESTORE_TLS;
            FileStoreRecordCipherDetails(FS, FS->S);
            Tempstr=STREAMReadDocument(Tempstr, S);
            UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
            FileBin_ParseItem(Items, Tempstr);
            STREAMClose(S);
        }
    }

    Destroy(Tempstr);
    Destroy(URL);
    return(Items);
}


static int FileBin_Info(TFileStore *FS)
{
    char *Tempstr=NULL;
    PARSER *P;
    STREAM *S;
    const char *ptr;
    int RetVal=FALSE;


    Tempstr=MCopyStr(Tempstr, "https://filebin.net/", FS->CurrDir, NULL);
    S=STREAMOpen(Tempstr, "r Accept=application/json");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        if (StrValid(Tempstr))
        {
            UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
            P=ParserParseDocument("json", Tempstr);
            SetVar(FS->Vars, "filebin:created", ParserGetValue(P, "/bin/created_at"));
            SetVar(FS->Vars, "filebin:updated", ParserGetValue(P, "/bin/updated_at"));
            SetVar(FS->Vars, "filebin:expires", ParserGetValue(P, "/bin/expired_at"));
            SetVar(FS->Vars, "filebin:hcreated", ParserGetValue(P, "/bin/created_at_relative"));
            SetVar(FS->Vars, "filebin:hupdated", ParserGetValue(P, "/bin/updated_at_relative"));
            SetVar(FS->Vars, "filebin:hexpires", ParserGetValue(P, "/bin/expired_at_relative"));
            RetVal=TRUE;
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(RetVal);
}


static STREAM *FileBin_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    char *URL=NULL, *Tempstr=NULL;
    const char *ptr;
    STREAM *S;

    Tempstr=HTTPQuote(Tempstr, GetBasename(Path));
    URL=MCopyStr(URL, "https://filebin.net/", FS->CurrDir, "/", Tempstr, NULL);
    if (OpenFlags & XFER_FLAG_WRITE) Tempstr=FormatStr(Tempstr, "w Accept=application/json Content-Type=application/octet-stream cid=12345 Content-Length=%lld", Size);
    else Tempstr=CopyStr(Tempstr, "r Accept=application/json");

    UI_Output(UI_OUTPUT_DEBUG, "url: %s", URL);
    S=STREAMOpen(URL, Tempstr);

    Destroy(Tempstr);
    Destroy(URL);

    return(S);
}


static int FileBin_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL;

    STREAMWriteLine("\r\n", S);
    STREAMCommit(S);
    Tempstr=STREAMReadDocument(Tempstr, S);
    UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
    FileBin_ParseItem(FS->DirList, Tempstr);
    STREAMClose(S);

    Destroy(Tempstr);
}


static int FileBin_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}

static int FileBin_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


static int FileBin_Unlink(TFileStore *FS, const char *Path)
{
    char *URL=NULL, *Tempstr=NULL;
    const char *ptr;
    STREAM *S;
    int RetVal=FALSE;

    if (! StrValid(Path))
    {
        UI_Output(UI_OUTPUT_ERROR, "ERROR: %s", "Delete command lacks target.");
        UI_Output(UI_OUTPUT_ERROR, "  %s", "'rm <filename>' to delete a file.");
        UI_Output(UI_OUTPUT_ERROR, "  %s", "'rm .' to delete this entire bin and all its files.");
        return(FALSE);
    }

    if ( (strcmp(Path, ".")==0) || (strcmp(Path, FS->CurrDir)==0) ) URL=MCopyStr(URL, "https://filebin.net/", FS->CurrDir, NULL);
    else URL=MCopyStr(URL, "https://filebin.net/", FS->CurrDir, "/", GetBasename(Path), NULL);

    S=STREAMOpen(URL, "D");
    if (S)
    {
        if (strcmp(STREAMGetValue(S, "HTTP:ResponseCode"), "200")==0) RetVal=TRUE;
        Tempstr=STREAMReadDocument(Tempstr, S);
        UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
        STREAMClose(S);
    }
    Destroy(Tempstr);
    Destroy(URL);

    return(RetVal);
}


static int FileBin_Freeze(TFileStore *FS, const char *Path)
{
    char *URL=NULL, *Tempstr=NULL;
    const char *ptr;
    STREAM *S;
    int RetVal=FALSE;


    if ((! StrValid(Path)) || (strcmp(Path, ".")==0))
    {
        URL=MCopyStr(URL, "https://filebin.net/", FS->CurrDir, NULL);
        S=STREAMOpen(URL, "W");
        if (S)
        {
            STREAMCommit(S);
            if (strcmp(STREAMGetValue(S, "HTTP:ResponseCode"), "200")==0) RetVal=TRUE;
            Tempstr=STREAMReadDocument(Tempstr, S);
            UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
            STREAMClose(S);
        }
    }
    else UI_Output(UI_OUTPUT_ERROR, "ERROR: %s", "You can only lock a bin with 'lock .', no other paths are accepted.");


    Destroy(Tempstr);
    Destroy(URL);

    return(RetVal);
}





static char *FileBin_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    int total, avail, used;

    RetStr=CopyStr(RetStr, "");

    return(RetStr);
}


static int FileBin_Connect(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    int RetVal=FALSE;

    if (! SSLAvailable())
    {
        UI_Output(UI_OUTPUT_ERROR, "ERROR: %s", "SSL/TLS support not compiled in\n");
        return(FALSE);
    }

    FS->CurrDir=FileBin_BinName(FS->CurrDir, FS);
    if (FileBin_Info(FS))
    {
        UI_Output(0, "CURRENT BIN: %s  created:%s  updated:%s  expires:%s", FS->CurrDir, GetVar(FS->Vars, "filebin:hcreated"), GetVar(FS->Vars, "filebin:hupdated"), GetVar(FS->Vars, "filebin:hexpires"));
        UI_Output(0, "%s", "Filebin supports freezing an entire 'bin' with the 'freeze' command.");
        UI_Output(0, "%s", "A frozen bin is read-only and cannot be subsequently written to.");
        RetVal=TRUE;
    }

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetVal);

}


int FileBin_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS;

    FS->Connect=FileBin_Connect;
    FS->OpenFile=FileBin_OpenFile;
    FS->CloseFile=FileBin_CloseFile;
    FS->ReadBytes=FileBin_ReadBytes;
    FS->WriteBytes=FileBin_WriteBytes;
    FS->ListDir=FileBin_ListDir;
    FS->UnlinkPath=FileBin_Unlink;
    FS->Freeze=FileBin_Freeze;

    /*
    FS->GetValue=FileBin_GetValue;
        FS->FileInfo=FTP_FileInfo;
        FS->ChDir=FTP_ChDir;
    */

    return(TRUE);
}
