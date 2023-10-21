#include "http.h"
#include "fileitem.h"
#include "ui.h"
#include "webdav.h"
#include "list_content_type.h"
#include "errors_and_logging.h"
#include "password.h"

static char *HTTPSetConfig(char *RetStr, TFileStore *FS, const char *Method, int Depth, const char *ContentType, int ContentLength, const char *ExtraArgs)
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


STREAM *HTTPOpenURL(TFileStore *FS, const char *Method, const char *URL, const char *ExtraArgs, const char *ContentType, int ContentSize, int DavDepth)
{
    char *FullURL=NULL, *Args=NULL, *Tempstr=NULL;
    STREAM *S;

    Args=HTTPSetConfig(Args, FS, Method, DavDepth, ContentType, ContentSize, ExtraArgs);
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


int HTTPCheckResponseCode(STREAM *S)
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



int HTTPBasicCommand(TFileStore *FS, const char *Target, const char *Method, const char *ExtraArgs)
{
    int RetVal=FALSE, i;
    char *Tempstr=NULL, *Args=NULL, *URL=NULL;
    const char *ptr;
    STREAM *S;

    S=HTTPOpenURL(FS, Method, Target, ExtraArgs, "", 0, 0);
    if (S)
    {
        RetVal=HTTPCheckResponseCode(S);
        if (RetVal != TRUE)
        {
            Tempstr=MCopyStr(Tempstr, STREAMGetValue(S, "HTTP:ResponseCode"), " ", STREAMGetValue(S, "HTTP:ResponseReason"), NULL);
            HandleEvent(FS, RetVal, Tempstr, "", "");
        }

        if (StrValid(STREAMGetValue(S, "HTTP:DAV"))) FS->Type=FILESTORE_WEBDAV;
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


int HTTP_Unlink_Path(TFileStore *FS, const char *Target)
{
    int result;

    result=HTTPBasicCommand(FS, Target, "DELETE", "");
    return(result);
}


int HTTP_MkDir(TFileStore *FS, const char *Path, int Mode)
{
    int result;

    result=HTTPBasicCommand(FS, Path, "MKCOL", "");
    return(result);
}


int HTTP_Rename_Path(TFileStore *FS, const char *Path, const char *NewPath)
{
    char *Tempstr=NULL, *URL=NULL;
    int result;

    URL=MCopyStr(URL, FS->URL, NewPath, NULL);
    Tempstr=MCopyStr(Tempstr, "Destination=", URL, " Depth=infinity", NULL);
    result=HTTPBasicCommand(FS, Path, "MOVE", Tempstr);

    Destroy(Tempstr);
    Destroy(URL);

    return(result);
}



int IsDownloadableURL(const char *URL)
{
    if (*URL=='#') return(FALSE);
    if (strncmp(URL, "javascript:", 11)==0) return(FALSE);
    return(TRUE);
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
        S=HTTPOpenURL(FS, "GET", Dir, "", "", 0, 0);
        if (S)
        {
            Tempstr=STREAMReadDocument(Tempstr, S);
            RetVal=HTTPCheckResponseCode(S);
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



ListNode *HTTP_ListDir(TFileStore *FS, const char *Dir)
{
    ListNode *FileList;

    FileList=ListCreate();
    HTTP_Internal_ListDir(FS, Dir, FileList);

    return(FileList);
}


STREAM *HTTP_OpenFile(TFileStore *FS, const char *Path, const char *OpenFlags, uint64_t Size)
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

    S=HTTPOpenURL(FS, Method, Path, "", "", Size, 0);
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


int HTTP_CloseFile(TFileStore *FS, STREAM *S)
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
            if (HTTPCheckResponseCode(S) != TRUE) RetVal=FALSE;
        }
        STREAMClose(S);
    }

    return(RetVal);
}


int HTTP_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


int HTTP_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


static int HTTP_OpenConnection(TFileStore *FS)
{
    int result=FALSE;

    result=HTTPBasicCommand(FS, "", "OPTIONS", "");
    if ((result != TRUE) && (! StrValid(FS->User)) ) result=HTTPBasicCommand(FS, "", "GET", "");
    if (result == TRUE)
    {
        result=HTTP_Internal_ListDir(FS, "/", NULL);
        if (result != TRUE) result=HTTP_Internal_ListDir(FS, "", NULL);
    }

    return(result);
}



int HTTP_Connect(TFileStore *FS)
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
    if (result == ERR_FORBID) HandleEvent(FS, 0, "(filestore): HTTP connect failed: Forbidden. ", FS->URL, "");
    else HandleEvent(FS, 0, "(filestore): HTTP connect failed.", FS->URL, "");

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
    FS->GetValue=WebDav_GetValue;
//FS->CurrDir=HTTP_RealPath(FS->CurrDir, ".", S);
}
