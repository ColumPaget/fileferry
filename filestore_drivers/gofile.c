#include "gdrive.h"
#include "../filestore_index.h"

static int GoFile_Info(TFileStore *FS)
{
    char *Tempstr=NULL;
    PARSER *P;
    STREAM *S;
    const char *ptr;
    int RetVal=FALSE;

    Tempstr=MCopyStr(Tempstr, "https://api.gofile.io/getAccountDetails?token=", FS->User, "&allDetails=true", NULL);
    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        FS->Flags |= FILESTORE_TLS;
        FileStoreRecordCipherDetails(FS, FS->S);
        Tempstr=STREAMReadDocument(Tempstr, S);
        if (StrValid(Tempstr))
        {
            P=ParserParseDocument("json", Tempstr);
            ptr=ParserGetValue(P, "/status");
            if (StrValid(ptr) && (strcmp(ptr, "ok")==0))
            {
                RetVal=TRUE;
                SetVar(FS->Vars, "RootFolder", ParserGetValue(P, "/data/rootFolder"));
                SetVar(FS->Vars, "UserEmail", ParserGetValue(P, "/data/email"));
                SetVar(FS->Vars, "UserTier", ParserGetValue(P, "/data/tier"));
                SetVar(FS->Vars, "TotalFiles", ParserGetValue(P, "/data/filesCountLimit"));
                SetVar(FS->Vars, "UsedFiles", ParserGetValue(P, "/data/filesCount"));
                SetVar(FS->Vars, "TotalDisk", ParserGetValue(P, "/data/totalSizeLimit"));
                SetVar(FS->Vars, "UsedDisk", ParserGetValue(P, "/data/totalSize"));
            }
            ParserItemsDestroy(P);
        }
        STREAMClose(S);
    }

    Tempstr=MCopyStr(Tempstr, "https://api.gofile.io/getServer?token=", FS->User, NULL);
    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        if (StrValid(Tempstr))
        {
            P=ParserParseDocument("json", Tempstr);
            ptr=ParserGetValue(P, "/status");
            if (StrValid(ptr) && (strcmp(ptr, "ok")==0))
            {
                RetVal=TRUE;
                SetVar(FS->Vars, "GoFileServer", ParserGetValue(P, "/data/server"));

            }
            ParserItemsDestroy(P);
        }
        STREAMClose(S);
    }


    Destroy(Tempstr);
    return(RetVal);
}


static STREAM *GoFile_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    char *URL=NULL, *Tempstr=NULL, *Boundary=NULL;
    char *Page=NULL;
    const char *ptr;
    STREAM *S;

    Boundary=FormatStr(Boundary, "----------------%x%x%x",time(NULL), getpid(), rand());
    Page=MCopyStr(Page, "--", Boundary, "\r\nContent-Disposition: form-data; name=\"token\"\r\nContent-Type: text/plain\r\n\r\n", FS->User, "\r\n", NULL);
    ptr=GetVar(FS->Vars, "Expire");
    if (StrValid(ptr)) Page=MCopyStr(Page, "--", Boundary, "\r\nContent-Disposition: form-data; name=\"expire\"\r\nContent-Type: text/plain\r\n\r\n", ptr, "\r\n", NULL);
    Page=MCatStr(Page, "--", Boundary, "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"", GetBasename(Path), "\"\r\nContent-Type: application/octet-stream\r\n\r\n", NULL);

    URL=MCopyStr(URL, "https://", GetVar(FS->Vars, "GoFileServer"), ".gofile.io/uploadFile", NULL);
    Tempstr=FormatStr(Tempstr, "w 'Content-Type=multipart/form-data; boundary=%s' Content-Length=%d", Boundary, StrLen(Page) + Size +StrLen(Boundary) +6);
    S=STREAMOpen(URL, Tempstr);
    if (S)
    {
        STREAMWriteLine(Page, S);
        SetVar(FS->Vars, "Boundary", Boundary);
    }

    Destroy(Tempstr);
    Destroy(Boundary);
    Destroy(URL);
    return(S);
}


static int GoFile_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL;
    PARSER *P;
    ListNode *FP;
    const char *ptr;

    STREAMWriteLine("\r\n--", S);
    STREAMWriteLine(GetVar(FS->Vars, "Boundary"), S);
    STREAMWriteLine("--\r\n", S);
    STREAMCommit(S);
    Tempstr=STREAMReadDocument(Tempstr, S);

    P=ParserParseDocument("json", Tempstr);
    ptr=ParserGetValue(P, "/status");
    if (StrValid(ptr) && (strcmp(ptr, "ok")==0))
    {
        FP=ParserOpenItem(P, "/data");
        Tempstr=CopyStr(Tempstr, ParserGetValue(P, "fileId"));
        Tempstr=MCatStr(Tempstr, ":", ParserGetValue(P, "code"));
        FileStoreIndexAdd(FS, ParserGetValue(P, "fileName"), Tempstr, 0, time(NULL), time(NULL), 0);
    }
    printf("%s\n", Tempstr);
    ParserItemsDestroy(P);
    STREAMClose(S);

    Destroy(Tempstr);
}


static int GoFile_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}

static int GoFile_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


static int GoFile_Unlink(TFileStore *FS, const char *Path)
{
    int RetVal=FALSE;

    return(RetVal);
}


static int GoFile_Rename(TFileStore *FS, const char *OldPath, const char *NewPath)
{
    STREAM *S;

    return(TRUE);
}


static int GoFile_Copy(TFileStore *FS, const char *OldPath, const char *NewPath)
{

    return(TRUE);
}



static int GoFile_MkDir(TFileStore *FS, const char *Path)
{

    return(TRUE);
}



static ListNode *GoFile_ListDir(TFileStore *FS, const char *Path)
{
    ListNode *Items;
    char *Tempstr=NULL;
    const char *ptr;
    PARSER *P;
    STREAM *S;

    Tempstr=MCopyStr(Tempstr, "https://api.gofile.io/getContent?contentId=", GetVar(FS->Vars, "RootFolder"), "&token=", FS->User, NULL);
    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        P=ParserParseDocument("json", Tempstr);
        ptr=ParserGetValue(P, "/status");
        if (strcmp(ptr, "ok") != 0)
        {
            Items=ListCreate();
        }
        else
        {
            Items=FileStoreIndexList(FS, "");
        }
        printf("%s\n", Tempstr);
        ParserItemsDestroy(P);
        STREAMClose(S);
    }

    Destroy(Tempstr);
    return(Items);
}



static char *GoFile_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    int total, avail, used;

    RetStr=CopyStr(RetStr, "");
    if (GoFile_Info(FS))
    {
        total=atof(GetVar(FS->Vars, "TotalDisk"));
        used=atof(GetVar(FS->Vars, "UsedDisk"));

        RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f", total, total-used, used);
    }

    return(RetStr);
}


static int GoFile_Connect(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    char *ptr;
    int RetVal=FALSE;
    STREAM *S;
    OAUTH *OauthCtx;

//    LibUsefulSetValue("HTTP:Debug", "Y");

    if (! SSLAvailable())
    {
        printf("ERROR: SSL/TLS support not compiled in\n");
        return(FALSE);
    }


    if (GoFile_Info(FS)) RetVal=TRUE;

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetVal);

}



int GoFile_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS;

    FS->ListDir=GoFile_ListDir;

    FS->Connect=GoFile_Connect;
    FS->OpenFile=GoFile_OpenFile;
    FS->CloseFile=GoFile_CloseFile;
    FS->ReadBytes=GoFile_ReadBytes;
    FS->WriteBytes=GoFile_WriteBytes;
    FS->GetValue=GoFile_GetValue;

    /*
    FS->UnlinkPath=GoFile_Unlink;
    FS->RenamePath=GoFile_Rename;
    FS->CopyPath=GoFile_Copy;
    FS->MkDir=GoFile_MkDir;

        FS->FileInfo=FTP_FileInfo;
        FS->ChDir=FTP_ChDir;
    */

    return(TRUE);
}
