#include "http.h"
#include "webdav.h"
#include "../fileitem.h"
#include "../ui.h"
#include "../list_content_type.h"
#include "../errors_and_logging.h"
#include "../password.h"

static char *HTTP_SetConfig(char *RetStr, TFileStore *FS, const char *Method, int Depth, const char *ContentType, int ContentLength, const char *ExtraArgs)
{
    char *Tempstr=NULL;

    RetStr=MCopyStr(RetStr, " method=", Method, NULL);
    if (ContentLength > 0)
    {
        RetStr=MCatStr(RetStr, " Content-Type=", ContentType, NULL);
        Tempstr=FormatStr(Tempstr, "%d", ContentLength);
        RetStr=MCatStr(RetStr, " Content-Length=", Tempstr, NULL);
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


STREAM *HTTP_OpenURL(TFileStore *FS, const char *Method, const char *URL, const char *ExtraArgs, const char *ContentType, int ContentSize, int DavDepth)
{
    char *FullURL=NULL, *Args=NULL, *Tempstr=NULL;
    STREAM *S;

    Args=HTTP_SetConfig(Args, FS, Method, DavDepth, ContentType, ContentSize, ExtraArgs);
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

    Destroy(Tempstr);
    Destroy(FullURL);
    Destroy(Args);

    return(S);
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
        case 204: //no content, means item was deleted
            RetVal=TRUE;
            break;
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




int HTTP_BasicCommand(TFileStore *FS, const char *Target, const char *Method, const char *ExtraArgs)
{
    int RetVal=FALSE, i;
    char *Tempstr=NULL, *Args=NULL, *URL=NULL;
    const char *ptr;
    STREAM *S;

    S=HTTP_OpenURL(FS, Method, Target, ExtraArgs, "", 0, 0);
    if (S)
    {
        RetVal=HTTP_CheckResponseCode(S);
        if (RetVal != TRUE)
        {
            Tempstr=MCopyStr(Tempstr, STREAMGetValue(S, "HTTP:ResponseCode"), " ", STREAMGetValue(S, "HTTP:ResponseReason"), NULL);
            HandleEvent(FS, RetVal, Tempstr, "", "");
        }

        if (StrValid(STREAMGetValue(S, "HTTP:DAV")))
        {
            FS->Type=FILESTORE_WEBDAV;
            FS->GetValue=WebDav_GetValue;
        }

        FileStoreRecordCipherDetails(FS, S);

        ptr=STREAMGetValue(S, "HTTP:Date");
        if (StrValid(ptr)) FS->TimeOffset=DateStrToSecs("%a, %d %b %Y %H:%M:%S", ptr, NULL) - time(NULL);
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
        S=HTTP_OpenURL(FS, "GET", Dir, "", "", 0, 0);
        if (S)
        {
            Tempstr=STREAMReadDocument(Tempstr, S);
            RetVal=HTTP_CheckResponseCode(S);
            if (RetVal==TRUE)
            {
                ptr=STREAMGetValue(S, "HTTP:Content-Type");
                if (StrValid(ptr)) FileListForContentType(FileList, Tempstr, ptr);
            }
            STREAMClose(S);
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


static STREAM *HTTP_OpenFile(TFileStore *FS, const char *Path, const char *OpenFlags, uint64_t Size)
{
    char *Tempstr=NULL, *Method=NULL, *Args=NULL;
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    STREAM *S;

    Method=CopyStr(Method, "GET");

    for (ptr=OpenFlags; *ptr != '\0'; ptr++)
    {
        if (*ptr=='w') Method=CopyStr(Method, "PUT");
        if (*ptr==' ') break;
    }

    S=HTTP_OpenURL(FS, Method, Path, "", "", Size, 0);
    if (S)
    {
        if (strcmp(Method, "GET")==0)
        {
            Tempstr=MCopyStr(Tempstr, STREAMGetValue(S, "HTTP:ResponseCode"), " ", STREAMGetValue(S, "HTTP:ResponseReason"), "\n", NULL);
            fprintf(stderr, Tempstr);
        }
        STREAMSetValue(S, "HTTP:Method", Method);
    }

    if (! S) HandleEvent(FS, 0, Path, "HTTP file open failed.", "");

    Destroy(Tempstr);
    Destroy(Method);
    Destroy(Name);
    Destroy(Value);
    Destroy(Args);
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
            STREAMFlush(S);
            STREAMCommit(S);
            if (HTTP_CheckResponseCode(S) != TRUE) RetVal=FALSE;
        }
        STREAMClose(S);
    }

    return(RetVal);
}


static int HTTP_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


static int HTTP_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


static int HTTP_OpenConnection(TFileStore *FS)
{
    int result=FALSE;

    result=HTTP_BasicCommand(FS, "", "OPTIONS", "");
    if ((result != TRUE) && (! StrValid(FS->User)) ) result=HTTP_BasicCommand(FS, "", "GET", "");
    if (result == TRUE)
    {
        result=HTTP_Internal_ListDir(FS, "/", NULL);
        if (result != TRUE) result=HTTP_Internal_ListDir(FS, "", NULL);
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

    if (result==TRUE) return(TRUE);
    if (result == ERR_FORBID) HandleEvent(FS, 0, "$(filestore): HTTP connect failed: Forbidden. ", FS->URL, "");
    else HandleEvent(FS, 0, "$(filestore): HTTP connect failed.", FS->URL, "");

    return(FALSE);
}


char *HTTP_GetHash(char *RetStr, TFileStore *FS, const char *Path, const char *HashType)
{
    STREAM *S;
    char *Tempstr=NULL;

    RetStr=CopyStr(RetStr, "");
    Tempstr=MCopyStr(Tempstr, "Want-digest=", HashType, " ", NULL);
    Tempstr=MCatStr(Tempstr, "Want-content-digest=", HashType, " ", NULL);

    S=HTTP_OpenURL(FS, "HEAD", Path, Tempstr, "", 0, 0);
    if (S)
    {
        if (HTTP_CheckResponseCode(S))
        {
            printf("HTTP GH: %s %s\n", STREAMGetValue(S, "Content-Digest"), STREAMGetValue(S, "Digest"));
            if (! StrValid(RetStr)) RetStr=CopyStr(RetStr, STREAMGetValue(S, "Content-Digest"));
            if (! StrValid(RetStr)) RetStr=CopyStr(RetStr, STREAMGetValue(S, "Digest"));
            if ( (! StrValid(RetStr)) && (strcmp(HashType, "md5")==0) ) RetStr=CopyStr(RetStr, STREAMGetValue(S, "HTTP:Content-MD5"));
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(RetStr);
}


char *HTTP_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=CopyStr(RetStr, "");

    if (strcasecmp(ValName, "md5")==0) RetStr=HTTP_GetHash(RetStr, FS, Path, "md5");
    if (strcasecmp(ValName, "sha")==0) RetStr=HTTP_GetHash(RetStr, FS, Path, "sha");
    if (strcasecmp(ValName, "sha256")==0) RetStr=HTTP_GetHash(RetStr, FS, Path, "sha-256");

    return(RetStr);
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
//FS->CurrDir=HTTP_RealPath(FS->CurrDir, ".", S);
}
