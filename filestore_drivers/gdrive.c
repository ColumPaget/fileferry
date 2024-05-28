#include "gdrive.h"
#include "http.h"
#include "../ui.h"

#define GOOGLE_CLIENT_ID "596195748702.apps.googleusercontent.com"
#define GOOGLE_CLIENT_SECRET "YDYv4JZMI3umD80S7Xh_WNJV"


/*
static STREAM *GDrive_Connect(TFileStore *FS, const char *Path, )
{

}
*/


static char *CreateItemFormatJSON(char *RetStr, TFileStore *FS, const char *Path, const char *MimeType)
{
    const char *ptr;

    RetStr=MCopyStr(RetStr, "{\"name\": \"", GetBasename(Path), "\",\"mimeType\": \"", MimeType, "\"", NULL);
    ptr=strrchr(FS->CurrDir, '/');
    if (*ptr == '/') ptr++;
    if (StrValid(ptr))
    {
        RetStr=MCatStr(RetStr, ",\"parents\": [\"", ptr, "\"]", NULL);
    }
    RetStr=CatStr(RetStr, "}");

    return(RetStr);
}

static STREAM *GDrive_OpenFile(TFileStore *FS, const char *Path, const char *OpenFlags, uint64_t Size)
{
    STREAM *S;
    char *URL=NULL, *Tempstr=NULL, *PostData=NULL;
    PARSER *P;

    if (StrValid(OpenFlags) && (*OpenFlags=='w'))
    {
        PostData=CreateItemFormatJSON(PostData, FS, Path, "application/octet-stream");
        URL=MCopyStr(URL, "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable", NULL);
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' 'Content-Type=application/json; charset=UTF-8' Content-Length=%d", FS->Pass, StrLen(PostData));
        S=STREAMOpen(URL, Tempstr);
        if (S)
        {
            STREAMWriteLine(PostData, S);
            STREAMCommit(S);
            Tempstr=STREAMReadDocument(Tempstr, S);
            URL=CopyStr(URL, STREAMGetValue(S, "HTTP:Location"));
            STREAMClose(S);

            Tempstr=FormatStr(Tempstr, "W Authorization='Bearer %s' 'Content-Length=%lld'", FS->Pass, Size);
            S=STREAMOpen(URL, Tempstr);
        }
    }
    else
    {
        URL=MCopyStr(URL, "https://www.googleapis.com/drive/v3/files", Path, "?alt=media", NULL);
        Tempstr=MCopyStr(Tempstr, "r Authorization='Bearer ", FS->Pass, "'", NULL);
        S=STREAMOpen(URL, Tempstr);
    }


    Destroy(PostData);
    Destroy(Tempstr);
    Destroy(URL);

    return(S);
}


static int GDrive_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL;

    STREAMWriteLine("\r\n", S);
    STREAMCommit(S);
    Tempstr=STREAMReadDocument(Tempstr, S);
    STREAMClose(S);

    Destroy(Tempstr);
    return(TRUE);
}


static int GDrive_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}

static int GDrive_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


static int GDrive_Unlink(TFileStore *FS, const char *Path)
{
    STREAM *S;
    char *URL=NULL, *Tempstr=NULL;
    int RetVal=FALSE;

    URL=MCopyStr(URL, "https://www.googleapis.com/drive/v3/files", Path, NULL);
    Tempstr=MCopyStr(Tempstr, "D Authorization='Bearer ", FS->Pass, "'", NULL);
    S=STREAMOpen(URL, Tempstr);
    if (S) RetVal=TRUE;
    STREAMClose(S);

    Destroy(Tempstr);
    Destroy(URL);
    return(RetVal);
}





static int GDrive_Rename(TFileStore *FS, const char *OldPath, const char *NewPath)
{
    ListNode *Items, *Curr;
    TFileItem *FI;
    char *URL=NULL, *Tempstr=NULL, *PostData=NULL, *Parent=NULL;
    int RetVal=FALSE;
    STREAM *S;


    //Items=FS->ListDir(FS, "");
    Items=FS->DirList;
    Curr=ListFindNamedItem(Items, GetBasename(NewPath));
    if (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FI->type==FTYPE_DIR) Parent=CopyStr(Parent, FI->path);
    }

    Curr=ListFindNamedItem(Items, GetBasename(OldPath));
    if (Curr)
    {
        FI=(TFileItem *) Curr->Item;

        URL=MCopyStr(URL, "https://www.googleapis.com/drive/v3/files/", FI->path, NULL);
        if (StrValid(Parent))
        {
            URL=MCatStr(URL, "?removeParents=root&addParents=", Parent, NULL);
        }
        else PostData=MCopyStr(PostData, "{\"name\": \"", GetBasename(NewPath), "\"}", NULL);


        Tempstr=FormatStr(Tempstr, "P Authorization='Bearer %s' 'Content-Type=application/json; charset=UTF-8' Content-Length=%d", FS->Pass, StrLen(PostData));
        S=STREAMOpen(URL, Tempstr);
        if (S)
        {
            STREAMWriteLine(PostData, S);
            STREAMCommit(S);
            Tempstr=STREAMReadDocument(Tempstr, S);
            if (HTTP_CheckResponseCode(S)==TRUE) RetVal=TRUE;
            STREAMClose(S);
        }
    }

    Destroy(PostData);
    Destroy(Tempstr);
    Destroy(URL);

    return(RetVal);
}


static int GDrive_Copy(TFileStore *FS, const char *OldPath, const char *NewPath)
{
    ListNode *Items, *Curr;
    char *URL=NULL, *Tempstr=NULL, *PostData=NULL;
    TFileItem *FI;
    STREAM *S;

    Items=FS->ListDir(FS, "");
    Curr=ListFindNamedItem(Items, GetBasename(OldPath));
    if (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        URL=MCopyStr(URL, "https://www.googleapis.com/drive/v3/files/", FI->path, "/copy", NULL);
        PostData=MCopyStr(PostData, "{\"name\": \"", GetBasename(NewPath), "\"}", NULL);
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' 'Content-Type=application/json; charset=UTF-8' Content-Length=%d", FS->Pass, StrLen(PostData));
        S=STREAMOpen(URL, Tempstr);
        if (S)
        {
            STREAMWriteLine(PostData, S);
            STREAMCommit(S);
            Tempstr=STREAMReadDocument(Tempstr, S);
            STREAMClose(S);
        }

    }

    Destroy(PostData);
    Destroy(Tempstr);
    Destroy(URL);

    return(TRUE);
}



static int GDrive_MkDir(TFileStore *FS, const char *Path, int Mkdir)
{
    ListNode *Items, *Curr;
    char *URL=NULL, *Tempstr=NULL, *PostData=NULL;
    STREAM *S;

    URL=MCopyStr(URL, "https://www.googleapis.com/drive/v3/files", NULL);
    PostData=CreateItemFormatJSON(PostData, FS, Path, "application/vnd.google-apps.folder");
    printf("MKDIR: [%s] %s\n", URL, PostData);

    Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' 'Content-Type=application/json; charset=UTF-8' Content-Length=%d", FS->Pass, StrLen(PostData));
    S=STREAMOpen(URL, Tempstr);
    if (S)
    {
        STREAMWriteLine(PostData, S);
        STREAMCommit(S);
        Tempstr=STREAMReadDocument(Tempstr, S);
        STREAMClose(S);
    }


    Destroy(PostData);
    Destroy(Tempstr);
    Destroy(URL);

    return(TRUE);
}



static ListNode *GDrive_ListDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *URL=NULL;
    const char *ptr;
    STREAM *S;
    ListNode *Items, *FilesP, *Curr;
    TFileItem *FI;
    PARSER *P;
    uint64_t size;

    Items=ListCreate();
    Tempstr=MCopyStr(Tempstr, "r Authorization='Bearer ", FS->Pass, "'", NULL);

    if (strcmp(Path, "/")==0) URL=CopyStr(URL, "https://www.googleapis.com/drive/v3/files?fields=*&q='root'+in+parents");
    else
    {
        ptr=Path;
        if (*ptr == '/') ptr++;
        URL=MCopyStr(URL, "https://www.googleapis.com/drive/v3/files?fields=*&q='", ptr, "'+in+parents", NULL);
    }

    printf("URL: %s\n", Path);
    S=STREAMOpen(URL, Tempstr);
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        P=ParserParseDocument("json", Tempstr);
        fprintf(stderr, "%s\n", Tempstr);
        Curr=ParserOpenItem(P, "/files");
        while (Curr)
        {
            if (StrValid(ParserGetValue(Curr, "name")))
            {
                ptr=ParserGetValue(Curr, "mimeType");
                size=(uint64_t) strtoll(ParserGetValue(Curr, "size"), NULL, 10);
                if (ptr && (strcmp(ptr, "application/vnd.google-apps.folder")==0)) FI=FileItemCreate(ParserGetValue(Curr, "name"), FTYPE_DIR, size, 0777);
                else FI=FileItemCreate(ParserGetValue(Curr, "name"), FTYPE_FILE, size, 0666);

                FI->path=CopyStr(FI->path, ParserGetValue(Curr, "id"));
                FI->mtime=DateStrToSecs("%Y-%m-%dT%H:%M:%S.", ParserGetValue(Curr,"modifiedTime"), NULL);
                ptr=ParserGetValue(Curr, "ownedByMe");
                if (strcmp(ptr, "true")==0) FI->user=CopyStr(FI->user, "me");
                else FI->user=CopyStr(FI->user, "other");
                ptr=ParserGetValue(Curr, "md5Checksum");
                if (StrValid(ptr)) FI->hash=MCopyStr(FI->hash, "md5:", ptr, NULL);

                ListAddNamedItem(Items, FI->name, FI);
            }
            Curr=ListGetNext(Curr);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(URL);

    return(Items);
}


static OAUTH *GDriveOAuth(TFileStore *FS)
{
    char *Tempstr=NULL;
    OAUTH *Ctx;
    int result;

    Ctx=OAuthCreate("pkce", FS->URL, GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET, "https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v2/token");
    if (! OAuthLoad(Ctx, FS->URL, ""))
    {
        //OAuthSetRedirectURI(Ctx, "http://127.0.0.1:8989");
        OAuthStage1(Ctx, "https://accounts.google.com/o/oauth2/v2/auth");
        printf("GOTO BELOW URL IN A BROWSER. Login and/or grant access:\n%s\n", Ctx->VerifyURL);
        fflush(NULL);
        OAuthListen(Ctx, 8989, "https://oauth2.googleapis.com/token", 0);
    }

    FS->Pass=CopyStr(FS->Pass, Ctx->AccessToken);

//FS->Settings |= FS_OAUTH;
//ConfigFileSaveFileStore(FS);

    Destroy(Tempstr);

    return(Ctx);
}


/*
{
  "kind": "drive#about",
  "user": {
    "kind": "drive#user",
    "displayName": "Colum Paget",
    "photoLink": "https://lh3.googleusercontent.com/a/default-user=s64",
    "me": true,
    "permissionId": "00192312247281967394",
    "emailAddress": "colum.paget@googlemail.com"
  },
  "storageQuota": {
    "limit": "16106127360",
    "usage": "1041917535",
    "usageInDrive": "41746656",
    "usageInDriveTrash": "0"
  },
*/

static char *GDrive_Quota(char *RetStr, TFileStore *FS)
{
    char *Tempstr=NULL;
    STREAM *S;
    int RetVal=FALSE;
    PARSER *P;
    float total, avail, used;

    RetStr=CopyStr(RetStr, "");
    Tempstr=MCopyStr(Tempstr, "r Authorization='Bearer ", FS->Pass, "'", NULL);
    S=STREAMOpen("https://www.googleapis.com/drive/v3/about?fields=*", Tempstr);
    if (S)
    {

        if (strncmp(STREAMGetValue(S, "HTTP:ResponseCode"), "2", 1)==0)
        {
            Tempstr=STREAMReadDocument(Tempstr, S);
            P=ParserParseDocument("json", Tempstr);
            if (P)
            {
                total=atof(ParserGetValue(P, "/storageQuota/limit"));
                used=atof(ParserGetValue(P, "/storageQuota/usage"));
                RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f",total, total-used, used);
            }
            STREAMClose(S);
        }
    }

    Destroy(Tempstr);

    return(RetStr);
}



static char *GDrive_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=CopyStr(RetStr, "");
    if (strcmp(ValName, "DiskQuota")==0) RetStr=GDrive_Quota(RetStr, FS);

    return(RetStr);
}

static int GDrive_Info(TFileStore *FS)
{
    char *Tempstr=NULL;
    Tempstr=GDrive_GetValue(Tempstr, FS, "/", "DiskQuota");
    if (StrValid(Tempstr)) return(TRUE);
    Destroy(Tempstr);
    return(FALSE);
}


static int GDrive_ChDir(TFileStore *FS, const char *Path)
{
    if (strcmp(Path, "..")==0)
    {
        StrRTruncChar(FS->CurrDir, '/');
    }
    else FS->CurrDir=CatStr(FS->CurrDir, Path);

    return(TRUE);
}


static int GDrive_Connect(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    char *ptr;
    int RetVal=FALSE;
    STREAM *S;
    OAUTH *OauthCtx;

//    LibUsefulSetValue("HTTP:Debug", "Y");



    if (! SSLAvailable())
    {
        UI_Output(UI_OUTPUT_ERROR, "ERROR: %s", "SSL/TLS support not compiled in");
        return(FALSE);
    }

    OauthCtx=GDriveOAuth(FS);
    if (! GDrive_Info(FS))
    {
        OAuthRefresh(OauthCtx, "https://oauth2.googleapis.com/token");
        FS->Pass=CopyStr(FS->Pass, OauthCtx->AccessToken);
        if (GDrive_Info(FS)) RetVal=TRUE;
    } else RetVal=TRUE;

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetVal);

}


int GDrive_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS | FILESTORE_USAGE | FILESTORE_NOPATH;

    FS->Connect=GDrive_Connect;
    FS->ListDir=GDrive_ListDir;
    FS->OpenFile=GDrive_OpenFile;
    FS->CloseFile=GDrive_CloseFile;
    FS->ReadBytes=GDrive_ReadBytes;
    FS->WriteBytes=GDrive_WriteBytes;
    FS->UnlinkPath=GDrive_Unlink;
    FS->RenamePath=GDrive_Rename;
    FS->CopyPath=GDrive_Copy;
    FS->MkDir=GDrive_MkDir;
    FS->ChDir=GDrive_ChDir;
    FS->GetValue=GDrive_GetValue;

    return(TRUE);
}
