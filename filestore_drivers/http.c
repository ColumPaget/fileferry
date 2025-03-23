#include "http.h"
#include "webdav.h"
#include "../fileitem.h"
#include "../ui.h"
#include "../list_content_type.h"
#include "../errors_and_logging.h"
#include "../password.h"

static char *HTTP_SetConfig(char *RetStr, TFileStore *FS, const char *Method, int Depth, const char *ContentType, uint64_t ContentLength, uint64_t ContentOffset, const char *ExtraArgs)
{
    char *Tempstr=NULL;

    RetStr=MCopyStr(RetStr, " method=", Method, NULL);
    if (StrValid(ContentType)) RetStr=MCatStr(RetStr, " Content-Type='", ContentType, "'", NULL);

    if (ContentLength > 0)
    {
        Tempstr=FormatStr(Tempstr, " Content-Length='%llu'", (unsigned long long) ContentLength);
        RetStr=CatStr(RetStr, Tempstr);
    }

    if (ContentOffset > 0)
    {
        Tempstr=FormatStr(Tempstr, " Range='bytes=%llu-'", (unsigned long long) ContentOffset);
        RetStr=CatStr(RetStr, Tempstr);
    }


    if (StrValid(FS->User))  RetStr=MCatStr(RetStr, " user='", FS->User, "'", NULL);
    if (StrValid(FS->AuthCreds)) RetStr=MCatStr(RetStr, " password='", FS->AuthCreds, "'", NULL);

    if (strcmp(Method, "PROPFIND")==0)
    {
        Tempstr=FormatStr(Tempstr, " Depth=%d", Depth);
        RetStr=CatStr(RetStr, Tempstr);
    }

    RetStr=MCatStr(RetStr, " ", ExtraArgs, NULL);
    Destroy(Tempstr);

    return(RetStr);
}





int HTTP_CheckResponseCode(STREAM *S)
{
    const char *ptr;
    int RetVal=FALSE, code;

    ptr=STREAMGetValue(S, "HTTP:ResponseCode");
    if (StrValid(ptr))
    {
        code=strtol(ptr, NULL, 10);
        switch (code)
        {
        case 200:
            RetVal=TRUE;
            break;
        case 201: //created, means item was created
            RetVal=TRUE;
            break;
        case 203: //'cached' or otherwise non-authoritive data
            RetVal=TRUE;
            break;
        case 204: //no content, means item was deleted
            RetVal=TRUE;
            break;
        case 206: //partial content, response to request for part of a file
            RetVal=TRUE;
        case 207: //multipart, response to propfind
            RetVal=TRUE;
            break;
        case 401:
            RetVal=ERR_FORBID;
            break;
        case 411:
            RetVal=ERR_TOOBIG;
            break;
        case 507:
            RetVal=ERR_FULL;
            break;
        }
    }

    return(RetVal);
}


int HTTP_CheckResponse(TFileStore *FS, STREAM *S)
{
    int result;
    char *Tempstr=NULL;

    if (! S)
    {
        HandleEvent(FS, UI_OUTPUT_ERROR, "Connect Failed", "", "");
        return(FALSE);
    }

    result=HTTP_CheckResponseCode(S);

    if (result == TRUE) return(TRUE);

    Tempstr=MCopyStr(Tempstr, STREAMGetValue(S, "HTTP:ResponseCode"), " ", STREAMGetValue(S, "HTTP:ResponseReason"), NULL);
    HandleEvent(FS, result, Tempstr, "", "");

    Destroy(Tempstr);
    return(result);
}


STREAM *HTTP_OpenURL(TFileStore *FS, const char *Method, const char *URL, const char *ExtraArgs, const char *ContentType, int ContentSize, int ContentOffset, int DavDepth)
{
    char *FullURL=NULL, *Args=NULL, *Tempstr=NULL;
    const char *ptr;
    STREAM *S;

    Args=HTTP_SetConfig(Args, FS, Method, DavDepth, ContentType, ContentSize, ContentOffset, ExtraArgs);
    Tempstr=MCopyStr(Tempstr, Method, " ", URL, NULL);
    HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, "", "");

    if (strncmp(URL, "http:", 5)==0) FullURL=CopyStr(FullURL, URL);
    else if (strncmp(URL, "https:", 6)==0) FullURL=CopyStr(FullURL, URL);
    else
    {
        Tempstr=HTTPQuoteChars(Tempstr, URL, "@ ");
        FullURL=FileStoreFullURL(FullURL, Tempstr, FS);
    }

    S=STREAMOpen(FullURL, Args);
    if (S)
    {
        if (strncmp(FullURL, "https:", 6)==0)
        {
            FS->Flags |= FILESTORE_TLS;
            FileStoreRecordCipherDetails(FS, S);
        }

        //we can learn/detect various things from the connection
        if (StrValid(STREAMGetValue(S, "HTTP:DAV")))
        {
            FS->Type=FILESTORE_WEBDAV;
            FS->GetValue=WebDav_GetValue;
        }

        ptr=STREAMGetValue(S, "HTTP:Accept-Ranges");
        if (StrValid(ptr))
        {
            if (strcasecmp(ptr, "none") !=0) FS->Flags |= FILESTORE_RESUME_TRANSFERS;
        }


        ptr=STREAMGetValue(S, "HTTP:Date");
        if (StrValid(ptr)) FS->TimeOffset=DateStrToSecs("%a, %d %b %Y %H:%M:%S", ptr, NULL) - time(NULL);
    }

    Destroy(Tempstr);
    Destroy(FullURL);
    Destroy(Args);

    return(S);
}



int HTTP_BasicCommand(TFileStore *FS, const char *Target, const char *Method, const char *ExtraArgs)
{
    int RetVal=FALSE, i;
    char *Tempstr=NULL, *Args=NULL, *URL=NULL;
    STREAM *S;

    S=HTTP_OpenURL(FS, Method, Target, ExtraArgs, "", 0, 0, 0);
    if (S)
    {
        RetVal=HTTP_CheckResponseCode(S);

        Tempstr=STREAMReadDocument(Tempstr, S);
        STREAMClose(S);
    }
    else HandleEvent(FS, ERR_NOCONNECT, Tempstr, "", "");

    Destroy(Tempstr);
    Destroy(Args);
    Destroy(URL);

    return(RetVal);
}


static int HTTP_Unlink_Path(TFileStore *FS, const char *Target)
{
    int result;

    result=HTTP_BasicCommand(FS, Target, "DELETE", "");
    return(result);
}


static int HTTP_MkDir(TFileStore *FS, const char *Path, int Mode)
{
    int result;

    result=HTTP_BasicCommand(FS, Path, "MKCOL", "");
    return(result);
}


static int HTTP_Rename_Path(TFileStore *FS, const char *Path, const char *NewPath)
{
    char *Tempstr=NULL, *URL=NULL;
    int result;

    URL=MCopyStr(URL, FS->URL, NewPath, NULL);
    Tempstr=MCopyStr(Tempstr, "Destination=", URL, " Depth=infinity", NULL);
    result=HTTP_BasicCommand(FS, Path, "MOVE", Tempstr);

    Destroy(Tempstr);
    Destroy(URL);

    return(result);
}




static int HTTP_Internal_ListDir(TFileStore *FS, const char *Dir, ListNode *FileList)
{
    char *Tempstr=NULL, *Args=NULL;
    const char *ptr;
    STREAM *S;
    int RetVal=FALSE;

    if (FS->Type==FILESTORE_WEBDAV) RetVal=Webdav_ListDir(FS, Dir, FileList);
    else
    {
        S=HTTP_OpenURL(FS, "GET", Dir, "", "", 0, 0, 0);
        if (HTTP_CheckResponse(FS, S)==TRUE)
        {
            Tempstr=STREAMReadDocument(Tempstr, S);
            ptr=STREAMGetValue(S, "HTTP:Content-Type");
            if (StrValid(ptr)) FileListForContentType(FileList, Tempstr, ptr);
            STREAMClose(S);
            RetVal=TRUE;
        }
    }

    Destroy(Tempstr);
    Destroy(Args);

    return(RetVal);
}



static ListNode *HTTP_ListDir(TFileStore *FS, const char *Dir)
{
    ListNode *FileList;

    FileList=ListCreate();
    HTTP_Internal_ListDir(FS, Dir, FileList);

    return(FileList);
}

char *HTTP_GetHash(char *RetStr, TFileStore *FS, const char *Path, const char *HashType)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL, *WantDigest=NULL, *WantContentDigest=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr, "");

    ptr=GetToken(HashType, ",", &Token, 0);
    while (ptr)
    {
        if (strcmp(Token, "sha256")==0) WantDigest=CatStr(WantDigest, "sha-256,");
        else if (strcmp(Token, "sha512")==0) WantDigest=CatStr(WantDigest, "sha-512,");
        else WantDigest=MCatStr(WantDigest, Token, ",", NULL);

        if (strcmp(Token, "sha256")==0) WantContentDigest=CatStr(WantContentDigest, "sha-256=1,");
        else if (strcmp(Token, "sha512")==0) WantContentDigest=CatStr(WantContentDigest, "sha-512=1,");
        else WantContentDigest=MCatStr(WantContentDigest, Token, "=1,", NULL);
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Tempstr=MCopyStr(Tempstr, "Want-digest=", WantDigest, " ", NULL);
    Tempstr=MCatStr(Tempstr, "Want-content-digest=", WantContentDigest, " ", NULL);


    S=HTTP_OpenURL(FS, "HEAD", Path, Tempstr, "", 0, 0, 0);
    if (S)
    {
        if (HTTP_CheckResponse(FS, S) ==TRUE)
        {
            RetStr=CopyStr(RetStr, "");
            if (! StrValid(RetStr)) RetStr=CopyStr(RetStr, STREAMGetValue(S, "HTTP:Content-Digest"));
            if (! StrValid(RetStr)) RetStr=CopyStr(RetStr, STREAMGetValue(S, "HTTP:Digest"));
            if ( (! StrValid(RetStr)) && (strcmp(HashType, "md5")==0) ) RetStr=CopyStr(RetStr, STREAMGetValue(S, "HTTP:Content-MD5"));
            if (! StrValid(RetStr)) RetStr=CopyStr(RetStr, STREAMGetValue(S, "HTTP:ETag"));
            STREAMClose(S);
        }
    }

    Destroy(WantContentDigest);
    Destroy(WantDigest);
    Destroy(Tempstr);
    Destroy(Token);

    return(RetStr);
}


char *HTTP_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=HTTP_GetHash(RetStr, FS, Path, ValName);
    return(RetStr);
}


static STREAM *HTTP_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    char *Method=NULL;
    STREAM *S;

    if (OpenFlags & XFER_FLAG_WRITE) Method=CopyStr(Method, "PUT");
    else Method=CopyStr(Method, "GET");

    S=HTTP_OpenURL(FS, Method, Path, "", "", Size, Offset, 0);
    if (S)
    {
        STREAMSetValue(S, "HTTP:Method", Method);

        if (strcmp(Method, "GET")==0)
        {
            if (HTTP_CheckResponse(FS, S) != TRUE)
            {
                STREAMClose(S);
                S=NULL;
            }
        }
    }
    else HandleEvent(FS, 0, Path, "HTTP file open failed. No connection.", "");

    Destroy(Method);

    return(S);
}


static int HTTP_CloseFile(TFileStore *FS, STREAM *S)
{
    const char *ptr;
    int RetVal=TRUE;

    if (S)
    {
        ptr=STREAMGetValue(S, "HTTP:Method");
        if (StrValid(ptr) && (strcmp(ptr, "PUT")==0))
        {
            STREAMWriteLine("\r\n", S);
            STREAMCommit(S);
            if (HTTP_CheckResponseCode(S) != TRUE) RetVal=FALSE;
        }
        STREAMClose(S);
    }

    return(RetVal);
}


static int HTTP_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    int result;

    result=STREAMReadBytes(S, Buffer, len);
    return(result);
}


static int HTTP_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


static int HTTP_OpenConnection(TFileStore *FS)
{
    int result=FALSE;

    result=HTTP_BasicCommand(FS, "", "OPTIONS", "");
    if (result != TRUE) result=HTTP_BasicCommand(FS, FS->CurrDir, "OPTIONS", "");
    if ((result != TRUE) && (! StrValid(FS->User)) ) result=HTTP_BasicCommand(FS, "", "GET", "");
    if (result == TRUE)
    {
        result=HTTP_Internal_ListDir(FS, "/", NULL);
        if (result != TRUE) result=HTTP_Internal_ListDir(FS, "", NULL);
        if (result != TRUE) result=HTTP_Internal_ListDir(FS, FS->CurrDir, NULL);
    }

    return(result);
}



static int HTTP_Connect(TFileStore *FS)
{
    const char *ptr;
    int result, len;

    result=HTTP_OpenConnection(FS);
    if (result == ERR_FORBID)
    {
        len=PasswordGet(FS, 0, &ptr);
        FS->AuthCreds=CopyStrLen(FS->AuthCreds, ptr, len);
        result=HTTP_OpenConnection(FS);
    }

    switch (result)
    {
    case TRUE:
        FileStoreTestFeatures(FS);
        return(TRUE);
        break;
    case ERR_FORBID:
        HandleEvent(FS, 0, "$(filestore): HTTP connect failed: Forbidden. ", FS->URL, "");
        break;
    default:
        HandleEvent(FS, 0, "$(filestore): HTTP connect failed.", FS->URL, "");
        break;
    }

    return(FALSE);
}



int HTTP_Attach(TFileStore *FS)
{
    FS->Type=FILESTORE_HTTP;
    FS->Flags |= FILESTORE_FOLDERS;
    FS->Connect=HTTP_Connect;
    FS->ListDir=HTTP_ListDir;
    FS->MkDir=HTTP_MkDir;
    FS->OpenFile=HTTP_OpenFile;
    FS->CloseFile=HTTP_CloseFile;
    FS->ReadBytes=HTTP_ReadBytes;
    FS->WriteBytes=HTTP_WriteBytes;
    FS->UnlinkPath=HTTP_Unlink_Path;
    FS->RenamePath=HTTP_Rename_Path;
    FS->GetValue=HTTP_GetValue;
    SetVar(FS->Vars, "HashTypes", "detect");
//FS->CurrDir=HTTP_RealPath(FS->CurrDir, ".", S);
}
