#include "synology.h"
#include "../filestore.h"
#include "../fileitem.h"
#include "../ui.h"
#include "../password.h"
#include "../errors_and_logging.h"
#include "../settings.h"




static int SYNO_ReadReply(TFileStore *FS, STREAM *S, PARSER **JSON)
{
    char *Tempstr=NULL;
    const char *ptr;
    int RetVal=FALSE;

    Tempstr=STREAMReadDocument(Tempstr, S);
    HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, "", "");
    *JSON=ParserParseDocument("json", Tempstr);
    ptr=ParserGetValue(*JSON, "success");
    if (StrValid(ptr) && (strcasecmp(ptr, "true")==0)) RetVal=TRUE;
    Destroy(Tempstr);

    return(RetVal);
}



static char *SYNO_FormatLoginError(char *RetStr, const char *Prefix, PARSER *JSON)
{
    const char *ptr;

    RetStr=MCopyStr(RetStr, Prefix, " ", NULL);
    ptr=ParserGetValue(JSON, "error/code");
    if (StrValid(ptr))
    {
        RetStr=MCatStr(RetStr, "error: ", ptr, " - ", NULL);
        switch (atoi(ptr))
        {
        case 400:
            RetStr=CatStr(RetStr, "No such account or incorrect password");
            break;
        case 401:
            RetStr=CatStr(RetStr, "Account disabled");
            break;
        case 402:
            RetStr=CatStr(RetStr, "Permission denied");
            break;
        case 403:
            RetStr=CatStr(RetStr, "2-step verification required");
            break;
        case 404:
            RetStr=CatStr(RetStr, "2-step verification failed");
            break;
        }
    }

    return(RetStr);
}


static char *SYNO_FormatFileError(char *RetStr, const char *Prefix, PARSER *JSON)
{
    const char *ptr;

    RetStr=MCopyStr(RetStr, Prefix, " ", NULL);
    ptr=ParserGetValue(JSON, "error/code");
    if (StrValid(ptr))
    {
        RetStr=MCatStr(RetStr, "error: ", ptr, " - ", NULL);
        switch (atoi(ptr))
        {
        case 101:
            RetStr=CatStr(RetStr, "Missing API version or method");
            break;

        case 102:
            RetStr=CatStr(RetStr, "Requested API does not exist");
            break;

        case 103:
            RetStr=CatStr(RetStr, "Requested method does not exist");
            break;

        case 104:
            RetStr=CatStr(RetStr, "Requested version does not support request");
            break;

        case 105:
            RetStr=CatStr(RetStr, "Session lacks permission");
            break;

        case 400:
            RetStr=CatStr(RetStr, "Invalid parameter");
            break;
        case 401:
            RetStr=CatStr(RetStr, "Unknown error");
            break;
        case 402:
            RetStr=CatStr(RetStr, "System busy");
            break;
        case 403:
            RetStr=CatStr(RetStr, "Invalid user for this operation");
            break;
        case 404:
            RetStr=CatStr(RetStr, "Invalid group for this operation");
            break;
        case 405:
            RetStr=CatStr(RetStr, "Invalid user and group for this operation");
            break;
        case 406:
            RetStr=CatStr(RetStr, "Account lookup failed");
            break;
        case 407:
            RetStr=CatStr(RetStr, "Operation not permitted");
            break;
        case 408:
            RetStr=CatStr(RetStr, "No such file or directory");
            break;
        case 409:
            RetStr=CatStr(RetStr, "Non supported file system");
            break;
        case 410:
            RetStr=CatStr(RetStr, "Failed to connect to remote file system");
            break;
        case 411:
            RetStr=CatStr(RetStr, "Read only remote file system");
            break;
        case 412:
            RetStr=CatStr(RetStr, "Filename too long");
            break;
        case 413:
            RetStr=CatStr(RetStr, "Filename too long");
            break;
        case 414:
            RetStr=CatStr(RetStr, "File already exists");
            break;
        case 415:
            RetStr=CatStr(RetStr, "Disk quota exceeded");
            break;
        case 416:
            RetStr=CatStr(RetStr, "No space on device");
            break;
        case 417:
            RetStr=CatStr(RetStr, "Input/output error");
            break;
        case 418:
        case 419:
        case 420:
            RetStr=CatStr(RetStr, "Illegal name or path");
            break;
        case 421:
            RetStr=CatStr(RetStr, "Resource busy");
            break;
        }
    }

    return(RetStr);
}



static ListNode *SYNO_FileQuery(TFileStore *FS, const char *URL, const char *ReqTitle)
{
    char *Tempstr=NULL;
    const char *ptr;
    ListNode *JSON=NULL;
    STREAM *S;

    S=STREAMOpen(URL, "");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, "", "");
        JSON=ParserParseDocument("json", Tempstr);

        ptr=ParserGetValue(JSON, "success");
        if (strcmp(ptr, "true") !=0)
        {
            Tempstr=SYNO_FormatFileError(Tempstr, ReqTitle, JSON);
            HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, FS->URL, "");
            ParserItemsDestroy(JSON);
            JSON=NULL;
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    return(JSON);
}


static int SYNO_HasFeature(TFileStore *FS, const char *Feature)
{
    char *Token=NULL;
    const char *ptr;
    int RetVal=FALSE;

    ptr=GetToken(FS->Features, "\\S", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        StripLeadingWhitespace(Token);
        if (strcmp(Token, Feature)==0)
        {
            RetVal=TRUE;
            break;
        }
        ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
    }

    Destroy(Token);

    return(RetVal);
}


static void SYNO_ListDirParseItem(ListNode *FileList, ListNode *Curr)
{
    const char *p_Name, *p_Path, *ptr;
    TFileItem *FI;
    unsigned long size;

    p_Name=ParserGetValue(Curr, "name");
    ptr=ParserGetValue(Curr, "additional/size");
    if (StrValid(ptr)) size=atoi(ptr);
    ptr=ParserGetValue(Curr, "isdir");

    if (StrValid(ptr) && (strcmp(ptr, "true")==0) ) FI=FileItemCreate(p_Name, FTYPE_DIR, size, 0);
    else FI=FileItemCreate(p_Name, FTYPE_FILE, size, 0);
    FI->path=CopyStr(FI->path, ParserGetValue(Curr, "path"));
    ptr=ParserGetValue(Curr, "additional/time/mtime");

    if (StrValid(ptr)) FI->mtime=atoi(ptr);
    ListAddNamedItem(FileList, FI->name, FI);
}



static ListNode *SYNO_ListQuery(TFileStore *FS, const char *Path, const char *Additional)
{
    char *URL=NULL, *Quoted=NULL, *Tempstr=NULL, *Token=NULL;
    PARSER *JSON;
    const char *ptr, *p_Method="list";

    //if path is blank, or is '/' then list shares
    if (StrLen(Path) < 2) p_Method="list_share";

    Tempstr=CopyStr(Tempstr, "");
    Quoted=HTTPQuote(Quoted, Path);
    ptr=GetToken(Additional, ",", &Token, 0);
    while (ptr)
    {
        if (StrValid(Tempstr)) Tempstr=MCatStr(Tempstr, ",\"", Token,"\"", NULL);
        else Tempstr=MCatStr(Tempstr, "\"", Token,"\"", NULL);
        ptr=GetToken(ptr, ",", &Token, 0);
    }
    Token=MCopyStr(Token, "[", Tempstr, "]", NULL);
    Tempstr=HTTPQuote(Tempstr, Token);

    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.List"), "?api=SYNO.FileStation.List&version=2&method=", p_Method, "&folder_path=", Quoted, "&additional=", Tempstr, NULL);

    JSON=SYNO_FileQuery(FS, URL, "file list:");

    Destroy(Quoted);
    Destroy(Tempstr);
    Destroy(Token);
    Destroy(URL);

    return(JSON);
}


static ListNode *SYNO_List(TFileStore *FS, const char *Path, const char *Additional)
{
    ListNode *JSON=NULL, *ShareList=NULL, *Files=NULL, *Curr;
    const char *ptr;

    JSON=SYNO_ListQuery(FS, Path, Additional);
    ptr=ParserGetValue(JSON, "success");
    if (strcmp(ptr, "true")==0)
    {
        Files=ListCreate();

        if (strcmp(Path, "/")==0) ShareList=ParserOpenItem(JSON, "/data/shares");
        else
        {
            ShareList=ParserOpenItem(JSON, "/data/files");
            if (! ShareList) ShareList=ParserOpenItem(JSON, "/data/shares");
        }

        Curr=ListGetNext(ShareList);
        while (Curr)
        {
            SYNO_ListDirParseItem(Files, Curr);
            Curr=ListGetNext(Curr);
        }
    }


    ParserItemsDestroy(JSON);

    return(Files);
}


static ListNode *SYNO_ListDir(TFileStore *FS, const char *Path)
{
    return(SYNO_List(FS, Path, "size,time,owner"));
}



static TFileItem *SYNO_FileInfo(TFileStore *FS, const char *Path)
{
    TFileItem *Item=NULL, *Found=NULL;
    ListNode *List, *Curr;
    char *Dir=NULL;

    Dir=CopyStr(Dir, Path);
    StrRTruncChar(Dir, '/');

    List=SYNO_ListDir(FS, Dir);
    Curr=ListGetNext(List);
    while (Curr)
    {
        Item=(TFileItem *) Curr->Item;
        if (strcmp(Item->name, GetBasename(Path))==0)
        {
            Found=(TFileItem *) Curr->Item;
            Curr->Item=NULL;
            break;
        }
        Curr=ListGetNext(Curr);
    }

    ListDestroy(List, FileItemDestroy);
    Destroy(Dir);

    return(Found);
}




static int SYNO_MkDir(TFileStore *FS, const char *Dir, int Mkdir)
{
    char *URL=NULL, *Quoted=NULL;
    ListNode *JSON;
    int result=FALSE;

    Quoted=HTTPQuote(Quoted, Dir);
    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.CreateFolder"), "?api=SYNO.FileStation.CreateFolder&version=2&method=create&folder_path=", FS->CurrDir, "&name=", GetBasename(Dir), NULL);

    JSON=SYNO_FileQuery(FS, URL, "mkdir:");
    if (JSON)
    {
        result=TRUE;
        ParserItemsDestroy(JSON);
    }

    Destroy(Quoted);
    Destroy(URL);

    return(result);
}




static int SYNO_RmDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    int result=FALSE;


    Destroy(Tempstr);

    return(result);
}


static int SYNO_Unlink(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *URL=NULL, *Quoted=NULL;
    ListNode *JSON;
    int result=FALSE;

    Quoted=HTTPQuote(Quoted, Path);
    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.Delete"), "?api=SYNO.FileStation.Delete&version=2&method=delete&path=", Quoted, NULL);

    JSON=SYNO_FileQuery(FS, URL, "unlink:");
    if (JSON)
    {
        result=TRUE;
        ParserItemsDestroy(JSON);
    }

    Destroy(Tempstr);
    Destroy(Quoted);
    Destroy(URL);

    return(result);
}


static char *SYNO_CopyMoveStart(char *RetStr, TFileStore *FS, const char *FromPath, const char *ToPath, int DeleteSrc)
{
    char *URL=NULL, *QuotedFrom=NULL, *QuotedTo=NULL, *Tempstr=NULL;
    ListNode *JSON;

    RetStr=CopyStr(RetStr, "");
    QuotedFrom=HTTPQuote(QuotedFrom, FromPath);
    QuotedTo=HTTPQuote(QuotedTo, ToPath);
    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.CopyMove"), "?api=SYNO.FileStation.CopyMove&version=2&method=start&path=", QuotedFrom, "&dest_folder_path=", QuotedTo, NULL);
    if (DeleteSrc) URL=CatStr(URL, "&remove_src=true");

    JSON=SYNO_FileQuery(FS, URL, "copy/move:");
    if (JSON)
    {
        RetStr=CopyStr(RetStr, ParserGetValue(JSON, "data/taskid"));
        ParserItemsDestroy(JSON);
    }

    Destroy(QuotedFrom);
    Destroy(QuotedTo);
    Destroy(Tempstr);
    Destroy(URL);



    return(RetStr);
}


static int SYNO_CopyMoveStatus(TFileStore *FS, const char *ID)
{
    char *URL=NULL, *QuotedFrom=NULL, *QuotedTo=NULL, *Tempstr=NULL;
    ListNode *JSON;
    int result=FALSE;

    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.CopyMove"), "?api=SYNO.FileStation.CopyMove&version=2&method=status&taskid=", ID, NULL);

    JSON=SYNO_FileQuery(FS, URL, "copy/move:");
    if (JSON)
    {
        Tempstr=CopyStr(Tempstr, ParserGetValue(JSON, "data/finished"));
        if (strcmp(Tempstr, "true")==0) result=TRUE;
        ParserItemsDestroy(JSON);
    }

    Destroy(QuotedFrom);
    Destroy(QuotedTo);
    Destroy(Tempstr);
    Destroy(URL);



    return(result);
}



static int SYNO_RenameFile(TFileStore *FS, const char *FromPath, const char *ToPath)
{
    char *URL=NULL, *QuotedFrom=NULL, *QuotedTo=NULL;
    ListNode *JSON;
    int result=FALSE;
    const char *ptr;

    QuotedFrom=HTTPQuote(QuotedFrom, FromPath);
    QuotedTo=HTTPQuote(QuotedTo, GetBasename(ToPath));

    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.Rename"), "?api=SYNO.FileStation.Rename&version=2&method=rename&path=", QuotedFrom, "&name=", QuotedTo, NULL);

    JSON=SYNO_FileQuery(FS, URL, "rename:");
    if (JSON)
    {
        result=TRUE;
        ParserItemsDestroy(JSON);
    }

    Destroy(QuotedFrom);
    Destroy(QuotedTo);
    Destroy(URL);



    return(result);
}


static int SYNO_CopyMove(TFileStore *FS, const char *FromPath, const char *ToPath, int DeleteSrc)
{
    const char *ptr;
    char *DestDir=NULL, *From=NULL, *ID=NULL;
    int result=FALSE;

    From=CopyStr(From, FromPath);
    ptr=strrchr(ToPath, '/');
    if (ptr)
    {
        DestDir=CopyStrLen(DestDir, ToPath, ptr-ToPath);
        if (strncmp(FromPath, ToPath, ptr-ToPath) !=0)
        {
            ID=SYNO_CopyMoveStart(ID, FS, FromPath, DestDir, DeleteSrc);
            if (StrValid(ID))
            {
                while (! SYNO_CopyMoveStatus(FS, ID));
                result=TRUE;
            }
            From=MCopyStr(From, DestDir, "/", GetBasename(FromPath), NULL);
        }
    }

    ptr=strrchr(ToPath, '/');
    if (ptr && (*ptr=='/')) ptr++;
    if (StrValid(ptr) && (strcmp(GetBasename(FromPath), ptr) !=0) )
    {
        result=SYNO_RenameFile(FS, From, ToPath);
    }

    Destroy(DestDir);
    Destroy(From);
    Destroy(ID);

    return(result);
}

static int SYNO_Rename(TFileStore *FS, const char *FromPath, const char *ToPath)
{
    return(SYNO_CopyMove(FS, FromPath, ToPath, TRUE));
}


static int SYNO_CopyPath(TFileStore *FS, const char *FromPath, const char *ToPath)
{
    return(SYNO_CopyMove(FS, FromPath, ToPath, FALSE));
}


static int SYNO_ChMod(TFileStore *FS, const char *Path, int Mode)
{
    char *Tempstr=NULL;
    int result=FALSE;


    DestroyString(Tempstr);

    return(result);
}


static char *SYNO_GetUsage(char *RetStr, TFileStore *FS)
{
    float total=0, avail=0, used=0;
    ListNode *JSON, *ShareList, *Curr;
    char *Token=NULL;
    const char *ptr;

    JSON=SYNO_ListQuery(FS, "/", "volume_status");

    ShareList=ParserOpenItem(JSON, "/data/shares");
    Curr=ListGetNext(ShareList);
    while (Curr)
    {
        ptr=ParserGetValue(Curr, "path");
        if ( StrValid(ptr) && (strncmp(ptr, FS->CurrDir, StrLen(ptr))==0) )
        {
            ptr=ParserGetValue(Curr, "additional/volume_status/totalspace");
            total=atof(ptr);
            ptr=ParserGetValue(Curr, "additional/volume_status/freespace");
            avail=atof(ptr);
            used=total-avail;
            RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f", total, avail, used);
        }
        Curr=ListGetNext(Curr);
    }

    ParserItemsDestroy(JSON);
    Destroy(Token);

    return(RetStr);
}


static char *SYNO_StartGetMD5(char *ID, TFileStore *FS, const char *Path)
{
    char *URL=NULL, *Quoted=NULL;
    ListNode *JSON;
    const char *ptr;
    STREAM *S;

    ID=CopyStr(ID, "");
    Quoted=HTTPQuote(Quoted, Path);
    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.MD5"), "?api=SYNO.FileStation.MD5&version=2&method=start&file_path=", Quoted, NULL);

    JSON=SYNO_FileQuery(FS, URL, "file md5:");
    if (JSON)
    {
        ID=CopyStr(ID, ParserGetValue(JSON, "data/taskid"));
        ParserItemsDestroy(JSON);
    }

    Destroy(Quoted);
    Destroy(URL);

    return(ID);
}



static char *SYNO_CheckGetMD5(char *MD5, TFileStore *FS, const char *ID)
{
    char *URL=NULL, *Quoted=NULL, *Tempstr=NULL;
    ListNode *JSON;
    const char *ptr;
    STREAM *S;

    MD5=CopyStr(MD5, "");
    Tempstr=MCopyStr(Tempstr, "\"", ID, "\"", NULL);
    Quoted=HTTPQuote(Quoted, Tempstr);
    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.MD5"), "?api=SYNO.FileStation.MD5&version=2&method=status&taskid=", Quoted, NULL);

    JSON=SYNO_FileQuery(FS, URL, "file md5:");
    if (JSON)
    {
        MD5=CopyStr(MD5, ParserGetValue(JSON, "data/md5"));
        ParserItemsDestroy(JSON);
    }

    Destroy(Quoted);
    Destroy(Tempstr);
    Destroy(URL);

    return(MD5);
}



static char *SYNO_GetMD5(char *RetStr, TFileStore *FS, const char *Path)
{
    char *ID=NULL;

    RetStr=CopyStr(RetStr, "");
    ID=SYNO_StartGetMD5(ID, FS, Path);
    if (StrValid(ID))
    {
        while (! StrValid(RetStr))
        {
            RetStr=SYNO_CheckGetMD5(RetStr, FS, ID);
        }
    }

    Destroy(ID);

    return(RetStr);
}



static char *SYNO_GetShareLink(char *RetStr, TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *URL=NULL, *Password=NULL;
    PARSER *JSON, *Items;
    ListNode *Curr;

    RetStr=CopyStr(RetStr, "");
    Tempstr=HTTPQuote(Tempstr, Path);
    Password=GetRandomAlphabetStr(Password, 16);

    URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.Sharing"), "?api=SYNO.FileStation.Sharing&version=2&method=create&path=", Tempstr, "&password=", Password, NULL);
    JSON=SYNO_FileQuery(FS, URL, "sharelink:");
    if (JSON)
    {
        Items=ParserOpenItem(JSON, "data/links");
        Curr=ListGetNext(Items);
        RetStr=MCopyStr(RetStr, ParserGetValue(Curr, "url"), "  password=", Password, NULL);
        ParserItemsDestroy(JSON);
    }

    Destroy(Password);
    Destroy(Tempstr);
    Destroy(URL);

    return(RetStr);
}


static char *SYNO_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    if (strcasecmp(ValName, "DiskQuota")==0) return(SYNO_GetUsage(RetStr, FS));
    if (strcasecmp(ValName, "ShareLink")==0) return(SYNO_GetShareLink(RetStr, FS, Path));
    if (strcasecmp(ValName, "md5")==0) return(SYNO_GetMD5(RetStr, FS, Path));

    RetStr=CopyStr(RetStr, "");
    return(RetStr);
}



static STREAM *SYNO_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    char *Tempstr=NULL, *Args=NULL, *URL=NULL, *Header=NULL, *Boundary=NULL;
    STREAM *S=NULL;
    uint64_t len;

    if (OpenFlags & XFER_FLAG_WRITE)
    {
        URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.Upload"), NULL);

        GenerateRandomBytes(&Boundary, 16, ENCODE_BASE64);
        Header=MCopyStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"api\"\r\n\r\nSYNO.FileStation.Upload\r\n", NULL);
        Header=MCatStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"version\"\r\n\r\n2\r\n", NULL);
        Header=MCatStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"method\"\r\n\r\nupload\r\n", NULL);
        Header=MCatStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"create_parents\"\r\n\r\ntrue\r\n", NULL);
        Tempstr=HTTPQuote(Tempstr, FS->CurrDir);
        Header=MCatStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"overwrite\"\r\n\r\ntrue\r\n", NULL);
        Header=MCatStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"path\"\r\n\r\n", FS->CurrDir, "\r\n", NULL);
        Header=MCatStr(Header, "--", Boundary, "\r\nContent-disposition: form-data; name=\"file\"; filename=\"", GetBasename(Path), "\"\r\nContent-type: application/octet-stream\r\n\r\n",  NULL);
        len=Size + StrLen(Header) + StrLen(Boundary) + 8;
        Args=FormatStr(Args, "w method=POST Content-type=multipart/form-data Content-length=%llu Content-type='multipart/form-data; boundary=%s'", len, Boundary);

        S=STREAMOpen(URL, Args);
        if (S)
        {
            STREAMWriteLine(Header, S);
            STREAMSetValue(S, "Boundary", Boundary);
        }
    }
    else if (OpenFlags & XFER_FLAG_THUMB)
    {
        Tempstr=HTTPQuote(Tempstr, Path);
        URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.Thumb"), "?api=SYNO.FileStation.Thumb&version=2&method=get&size=small&path=", Tempstr, NULL);
        S=STREAMOpen(URL, "");
    }
    else
    {
        Tempstr=HTTPQuote(Tempstr, Path);
        URL=MCopyStr(URL, FS->URL, "/", GetVar(FS->Vars, "SYNO.FileStation.Download"), "?api=SYNO.FileStation.Download&version=2&method=download&path=", Tempstr, "&mode=download", NULL);
        S=STREAMOpen(URL, "");
    }

    Destroy(Boundary);
    Destroy(Tempstr);
    Destroy(Header);
    Destroy(Args);
    Destroy(URL);


    return(S);
}



static int SYNO_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL;
    ListNode *JSON=NULL;

    if (StrValid(STREAMGetValue(S, "Boundary")))
    {
        Tempstr=MCopyStr(Tempstr, "\r\n--", STREAMGetValue(S, "Boundary"), "--\r\n\r\n", NULL);
        STREAMWriteLine(Tempstr, S);
        STREAMCommit(S);
        if (! SYNO_ReadReply(FS, S, &JSON))
        {
            Tempstr=SYNO_FormatFileError(Tempstr, "file open:", JSON);
            HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, FS->URL, "");
        }
    }

    STREAMClose(S);

    Destroy(Tempstr);
    return(TRUE);
}



static int SYNO_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


static int SYNO_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}




static int SYNO_Disconnect(TFileStore *FS)
{

    return(TRUE);
}







static int SYNO_Auth(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    const char *ptr;
    PARSER *JSON=NULL;
    int RetVal=FALSE;

    Tempstr=MCopyStr(Tempstr, FS->URL, Path, "?api=SYNO.API.Auth&version=3&method=login&account=", FS->User, "&passwd=", FS->Pass, "&session=FileStation&format=cookie", NULL);

    FS->S=STREAMOpen(Tempstr, "");
    if (FS->S)
    {
        if (SYNO_ReadReply(FS, FS->S, &JSON)) RetVal=TRUE;
        else
        {
            Tempstr=SYNO_FormatLoginError(Tempstr, "$(filestore): Login Failed", JSON);
            HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, FS->URL, "");
        }

        STREAMClose(FS->S);
    }

    Destroy(Tempstr);
    return(RetVal);
}


static PARSER *SYNO_InitAPI(TFileStore *FS)
{
    char *Tempstr=NULL;
    PARSER *JSON=NULL;

    Tempstr=MCopyStr(Tempstr, FS->URL, "query.cgi?api=SYNO.API.Info&version=1&method=query&query=all", NULL);
    FS->S=STREAMOpen(Tempstr, "");
    if (FS->S)
    {
        Tempstr=STREAMReadDocument(Tempstr, FS->S);
        HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, "", "");
        JSON=ParserParseDocument("json", Tempstr);
        STREAMClose(FS->S);
    }

    Destroy(Tempstr);
    return(JSON);
}


static int SYNO_RegisterFeature(TFileStore *FS, PARSER *JSON, const char *Feature)
{
    char *Tempstr=NULL;
    const char *ptr;
    int result=FALSE;

    Tempstr=MCopyStr(Tempstr, "data/", Feature, "/path", NULL);
    ptr=ParserGetValue(JSON, Tempstr);
    SetVar(FS->Vars, Feature, ptr);
    if (StrValid(ptr)) result=TRUE;

    Destroy(Tempstr);

    return(result);
}


static int SYNO_Connect(TFileStore *FS)
{
    PARSER *JSON;
    char *Tempstr=NULL;
    int RetVal=FALSE;

    //reformat URL
    if (strncmp(FS->URL, "synos:", 6)==0)
    {
        Tempstr=CopyStr(Tempstr, FS->URL);
        FS->URL=MCopyStr(FS->URL, "https:", Tempstr+6, "/webapi/", NULL);
    }
    else //if (strncmp(FS->URL, "syno:", 5)==0)
    {
        Tempstr=CopyStr(Tempstr, FS->URL);
        FS->URL=MCopyStr(FS->URL, "http:", Tempstr+5, "/webapi/", NULL);
    }

    JSON=SYNO_InitAPI(FS);
    if (JSON)
    {
        RetVal=SYNO_Auth(FS, ParserGetValue(JSON, "data/SYNO.API.Auth/path"));
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.List");
        if (SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.MD5")) SetVar(FS->Vars, "HashTypes", "md5");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.CreateFolder");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Delete");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Rename");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.CopyMove");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Upload");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Download");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Info");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Thumb");
        SYNO_RegisterFeature(FS, JSON, "SYNO.FileStation.Sharing");
    }
    else HandleEvent(FS, 0, "$(filestore): Connection Failed.", FS->URL, "");

    Destroy(Tempstr);
    return(RetVal);
}


int SYNO_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS;

    FS->Connect=SYNO_Connect;
    FS->ListDir=SYNO_ListDir;
    FS->FileInfo=SYNO_FileInfo;
    FS->MkDir=SYNO_MkDir;
    FS->UnlinkPath=SYNO_Unlink;
    FS->RenamePath=SYNO_Rename;
    FS->CopyPath=SYNO_CopyPath;
    FS->OpenFile=SYNO_OpenFile;
    FS->CloseFile=SYNO_CloseFile;
    FS->ReadBytes=SYNO_ReadBytes;
    FS->WriteBytes=SYNO_WriteBytes;
    FS->GetValue=SYNO_GetValue;
}
