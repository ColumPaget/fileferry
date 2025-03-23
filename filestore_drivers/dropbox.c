#include "dropbox.h"
#include "http.h"
#include "../filestore_dirlist.h"
#include "../errors_and_logging.h"

#define DROPBOX_CLIENT_ID "tachgefepatxzya"



static TFileItem *DropBoxParseFileItem(ListNode *JSONP)
{
    TFileItem *FI=NULL;
    const char *ptr;
    uint64_t size;
    int FType=FTYPE_FILE;


    if (StrValid(ParserGetValue(JSONP, "name")))
    {
        ptr=ParserGetValue(JSONP, ".tag");
        size=(uint64_t) strtoll(ParserGetValue(JSONP, "size"), NULL, 10);
        if (CompareStr(ptr, "folder")==0) FType=FTYPE_DIR;

        FI=FileItemCreate(ParserGetValue(JSONP, "name"), FType, size, 0666);
        FI->path=CopyStr(FI->path, ParserGetValue(JSONP, "path_lower"));
        FI->mtime=DateStrToSecs("%Y-%m-%dT%H:%M:%S.", ParserGetValue(JSONP,"client_modified"), NULL);
        FI->hash=MCopyStr(FI->hash, "dropboxhash:", ParserGetValue(JSONP, "content_hash"), NULL);
    }

    return(FI);
}


static char *DropBox_Transact(char *RetStr, TFileStore *FS, const char *Path, const char *Args)
{
    char *URL=NULL, *ConnectConfig=NULL, *Tempstr=NULL;
    const char *ptr;
    STREAM *S;
    int RetVal=FALSE;


    RetStr=CopyStr(RetStr, "");
    URL=MCopyStr(URL, "https://api.dropboxapi.com/2", Path, NULL);
    if (StrValid(Args))
    {
        ConnectConfig=FormatStr(ConnectConfig, "w Authorization='Bearer %s' Content-Type=application/json Content-Length=%d", FS->Pass, StrLen(Args));

        //this is not HTTP GET, it is a logging line that displays what we're doing
        Tempstr=MCopyStr(Tempstr, "$(filestore) GET ", URL, NULL);
    }
    else
    {
        ConnectConfig=FormatStr(ConnectConfig, "w Authorization='Bearer %s'", FS->Pass);

        //this is not HTTP PUT, it is a logging line that displays what we're doing
        Tempstr=MCopyStr(Tempstr, "$(filestore): PUT ", URL, NULL);
    }

    HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, "", "");

    S=STREAMOpen(URL, ConnectConfig);
    if (S)
    {
        FS->Flags |= FILESTORE_TLS;
        FileStoreRecordCipherDetails(FS, S);
        if (StrValid(Args)) STREAMWriteLine(Args, S);
        STREAMCommit(S);
        ptr=STREAMGetValue(S, "HTTP:ResponseCode");

        if (StrValid(ptr) && (*ptr=='2'))
        {
            RetStr=STREAMReadDocument(RetStr, S);
        }
        else
        {
            Tempstr=STREAMReadDocument(Tempstr, S);
            HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, URL, "");
            Destroy(RetStr);
            RetStr=NULL;
        }
    }
    else HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, "", "");

    Destroy(ConnectConfig);
    Destroy(Tempstr);
    Destroy(URL);

    return(RetStr);
}



static STREAM *DropBox_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    STREAM *S, *InfoS=NULL;
    PARSER *P;
    char *URL=NULL, *Tempstr=NULL;


    if (OpenFlags & XFER_FLAG_UPLOAD)
    {
        URL=MCopyStr(URL, "https://content.dropboxapi.com/2/files/upload_session/start");
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' Dropbox-API-Arg='{\"close\": false}' Content-Type=application/octet-stream Content-Length=0", FS->Pass);
    }
    else
    {
        URL=MCopyStr(URL, "https://content.dropboxapi.com/2/files/download");
        //beware, dropbox uses 'POST' for getting files!
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' Dropbox-API-Arg='{\"path\": \"%s\"}'", FS->Pass, Path);
    }


    S=STREAMOpen(URL, Tempstr);
    if (S)
    {
        //as we are using POST for everything, we must STREAMCommit
        STREAMCommit(S);
        if (HTTP_CheckResponse(FS, S))
        {
            if (OpenFlags & XFER_FLAG_UPLOAD)
            {
                Tempstr=STREAMReadDocument(Tempstr, S);
                P=ParserParseDocument("json", Tempstr);
                InfoS=STREAMCreate();
                STREAMSetValue(InfoS, "dropbox_sessionid", ParserGetValue(P, "session_id"));
                STREAMSetValue(InfoS, "destination_path", Path);
                STREAMSetValue(InfoS, "dropbox_transfer", "upload");
                ParserItemsDestroy(P);
                STREAMClose(S);
                S=InfoS;
            }
        }
        else
        {
            STREAMClose(S);
            S=NULL;
        }
    }


    Destroy(Tempstr);
    Destroy(URL);

    return(S);
}


static int DropBox_CloseFile(TFileStore *FS, STREAM *InfoS)
{
    char *Tempstr=NULL;
    TFileItem *FI, *tmpFI;
    PARSER *P;
    STREAM *S=NULL;

    Tempstr=CopyStr(Tempstr, STREAMGetValue(InfoS, "dropbox_transfer"));
    fflush(NULL);

    if (strcmp(Tempstr, "upload")==0)
    {
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' Dropbox-API-Arg='{\"commit\": {\"autorename\": true, \"mode\": \"add\", \"path\": \"%s\", \"strict_conflict\": false},  \"cursor\": {\"session_id\": \"%s\", \"offset\": %s}}' Content-Type=application/octet-stream Content-Length=0", FS->Pass, STREAMGetValue(InfoS, "destination_path"), STREAMGetValue(InfoS, "dropbox_sessionid"), STREAMGetValue(InfoS, "dropbox_transfer_size"));
        S=STREAMOpen("https://content.dropboxapi.com/2/files/upload_session/finish", Tempstr);
        if (S)
        {
            STREAMCommit(S);
            Tempstr=STREAMReadDocument(Tempstr, S);
            UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
            P=ParserParseDocument("json", Tempstr);
            tmpFI=DropBoxParseFileItem(P);
            if (tmpFI)
            {
                FI=FileStoreDirListUpdateItem(FS, tmpFI->type, tmpFI->name, tmpFI->size, tmpFI->perms);
                FI->hash=CopyStr(FI->hash, tmpFI->hash);
                FileItemDestroy(tmpFI);
            }

            STREAMClose(S);
        }
    }

    STREAMClose(InfoS);
    Destroy(Tempstr);

    return(TRUE);
}


static int DropBox_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


//Dropbox has a special API that uses 'upload sessions' where the upload is broken up into chunks pushed in seperate
//HTTP sessions, which is why this all looks a bit odd
static int DropBox_WriteBytes(TFileStore *FS, STREAM *InfoS, char *Buffer, uint64_t offset, uint32_t len)
{
    char *Tempstr=NULL;
    int result=STREAM_CLOSED;
    STREAM *S;

    Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' Dropbox-API-Arg='{\"cursor\": {\"session_id\": \"%s\", \"offset\": %llu}, \"close\": false}' Content-Type=application/octet-stream Content-Length=%lu", FS->Pass, STREAMGetValue(InfoS, "dropbox_sessionid"), offset, len);

    S=STREAMOpen("https://content.dropboxapi.com/2/files/upload_session/append_v2", Tempstr);
    if (S)
    {
        result=STREAMWriteBytes(S, Buffer, len);

        //for some weird reason this Flush is required, even though
        //STREAMCommit contains a flush. Without this the upload hangs.
        STREAMFlush(S);

        STREAMCommit(S);

        Tempstr=STREAMReadDocument(Tempstr, S);
        STREAMClose(S);

        Tempstr=FormatStr(Tempstr, "%llu", offset + len);
        STREAMSetValue(InfoS, "dropbox_transfer_size", Tempstr);
    }

    Destroy(Tempstr);

    return(result);
}



/*
These functions relate to the basic 'upload' feature of dropbox that's been replaced by 'upload_session'.
Upload is simple, but has limits on how large a file transfer can be.

static STREAM *DropBox_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    STREAM *S=NULL;
    PARSER *P;
    char *URL=NULL, *Tempstr=NULL;


    if (OpenFlags & XFER_FLAG_WRITE)
    {
        URL=MCopyStr(URL, "https://content.dropboxapi.com/2/files/upload");
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' Dropbox-API-Arg='{\"autorename\": true, \"mode\": \"add\", \"path\": \"%s\"}' Content-Type=application/octet-stream Content-Length=%llu", FS->Pass, Path, Size);
    }
    else
    {
        URL=MCopyStr(URL, "https://content.dropboxapi.com/2/files/download");
        //beware, dropbox uses 'POST' for getting files!
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' Dropbox-API-Arg='{\"path\": \"%s\"}'", FS->Pass, Path);
    }


    S=STREAMOpen(URL, Tempstr);
    if (S)
    {
        if (OpenFlags & XFER_FLAG_UPLOAD)
        {
            STREAMSetValue(S, "dropbox_transfer", "upload");
            STREAMSetValue(S, "destination_path", Path);
        }
        else
        {
            //we use POST even for getting files. Crazy
            STREAMCommit(S);

            if (! HTTP_CheckResponse(FS, S))
            {
                STREAMClose(S);
                S=NULL;
            }
        }
    }


    Destroy(Tempstr);
    Destroy(URL);

    return(S);
}


static int DropBox_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL;

    Tempstr=CopyStr(Tempstr, STREAMGetValue(S, "dropbox_transfer"));
    if (strcmp(Tempstr, "upload")==0)
    {
        STREAMWriteLine("\r\n", S);
        STREAMCommit(S);
        Tempstr=STREAMReadDocument(Tempstr, S);
        if (Settings->Flags & SETTING_DEBUG) fprintf(stderr, "%s\n", Tempstr);
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(TRUE);
}


static int DropBox_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}

static int DropBox_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}
*/


static int DropBox_Unlink(TFileStore *FS, const char *Path)
{
    STREAM *S;
    char *JSON=NULL, *Tempstr=NULL;
    int RetVal=FALSE;


    Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path,"\"}", NULL);
    JSON=DropBox_Transact(JSON, FS, "/files/delete_v2", Tempstr);
    if (StrValid(JSON)) RetVal=TRUE;

    Destroy(Tempstr);
    Destroy(JSON);

    return(RetVal);
}


static int DropBox_Rename(TFileStore *FS, const char *OldPath, const char *NewPath)
{
    ListNode *Items, *Curr;
    char *JSON=NULL, *Tempstr=NULL;
    TFileItem *FI;
    int RetVal=FALSE;

    Items=FS->ListDir(FS, "");
    Curr=ListFindNamedItem(Items, GetBasename(OldPath));
    if (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        Tempstr=MCopyStr(Tempstr, "{\"from_path\": \"", FI->path, "\",\"to_path\": \"", NewPath, "\",\"allow_shared_folder\": false,\"autorename\": false,\"allow_ownership_transfer\": false}",NULL);
        JSON=DropBox_Transact(JSON, FS, "/files/move", Tempstr);
        if (StrValid(JSON)) RetVal=TRUE;
    }

    Destroy(Tempstr);
    Destroy(JSON);

    return(RetVal);
}


static int DropBox_MkDir(TFileStore *FS, const char *Path, int Mkdir)
{
    STREAM *S;
    char *JSON=NULL, *Tempstr=NULL;
    int RetVal=FALSE;

    Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path,"\", \"autorename\": false}", NULL);
    JSON=DropBox_Transact(JSON, FS, "/files/create_folder_v2", Tempstr);
    if (StrValid(JSON)) RetVal=TRUE;

    Destroy(Tempstr);
    Destroy(JSON);

    return(RetVal);
}




static ListNode *DropBox_ListDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *JSON=NULL;
    ListNode *Items, *FilesP, *Curr;
    TFileItem *FI;
    PARSER *P;

//{\"path\": \"/Homework/math\",\"recursive\": false,\"include_media_info\": false,\"include_deleted\": false,\"include_has_explicit_shared_members\": false,\"include_mounted_folders\": true,\"include_non_downloadable_files\": true}
//
//{"entries": [{".tag": "file", "name": "Get Started with Dropbox Paper.url", "path_lower": "/get started with dropbox paper.url", "path_display": "/Get Started with Dropbox Paper.url", "id": "id:NY2cP0crwoAAAAAAAAAABg", "client_modified": "2020-04-08T22:34:21Z", "server_modified": "2020-04-08T22:34:21Z", "rev": "015a2cf1a0002b800000001bb1c96b0", "size": 240, "is_downloadable": true, "content_hash": "f40c1228343d7e2a632281c986dbb7af3491b9b63ddfd0eb10fee2c913f6cfa7"}, {".tag": "file", "name": "Get Started with Dropbox.pdf", "path_lower": "/get started with dropbox.pdf", "path_display": "/Get Started with Dropbox.pdf", "id": "id:NY2cP0crwoAAAAAAAAAABw", "client_modified": "2020-04-08T22:34:21Z", "server_modified": "2020-04-08T22:34:21Z", "rev": "015a2cf1a0002b900000001bb1c96b0", "size": 1102331, "is_downloadable": true, "content_hash": "f7ad488deb7d81790340ecd676fe6e47f0a6064fb99b982685b752d58611c1cb"}], "cursor": "AAGCmtkBfAAxfYBp8WQUxW5pxUiT8YqEgBZkX5XJcUPP9Jn30T1dEKOkZtTKXzlUHPqNnKmEkXy9CrRDZmfHrnwyl68rwi6oEHVcwa4l8-BT_nHsZ6chH00bnqOZtgWymatxb_Lyh3EL4JlJ_ENLOYje6Y3OWGqOmE3DP_RfbV1Z-KBFLUqO2ARoQi9MfavtEi8", "has_more": false}


    Items=ListCreate();

    if (strcmp(Path, "/")==0) Tempstr=CopyStr(Tempstr, "{\"path\": \"\",\"recursive\": false}");
    else Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path,"\",\"recursive\": false}", NULL);

    JSON=DropBox_Transact(JSON, FS, "/files/list_folder", Tempstr);
    if (Settings->Flags & SETTING_DEBUG) fprintf(stderr, "%s\n", JSON);
    P=ParserParseDocument("json", JSON);
    if (P)
    {
        Curr=ParserOpenItem(P, "entries");
        while (Curr)
        {
            FI=DropBoxParseFileItem(Curr);
            if (FI) ListAddNamedItem(Items, FI->name, FI);
            Curr=ListGetNext(Curr);
        }
        ParserItemsDestroy(P);
    }

    return(Items);
}






/*
 * {"account_id": "dbid:AAATkllVLE8-FjpRB3Wz4ALOPO7azZ9pHGU", "name": {"given_name": "Colum", "surname": "Paget", "familiar_name": "Colum", "display_name": "Colum Paget", "abbreviated_name": "CP"}, "email": "colum.paget@gmail.com", "email_verified": true, "disabled": false, "country": "GB", "locale": "en", "referral_link": "https://www.dropbox.com/referrals/AABhVoK3mS6-Jnaeg0pYZyt6NxPh_oaQFqk?src=app9-11426688", "is_paired": false, "account_type": {".tag": "basic"}, "root_info": {".tag": "user", "root_namespace_id": "7434180272", "home_namespace_id": "7434180272"}}

 {"used": 1102571, "allocation": {".tag": "individual", "allocated": 2147483648}}

*/

static int DropBox_Info(TFileStore *FS)
{
    char *Tempstr=NULL;
    int RetVal=FALSE;
    PARSER *P;

    Tempstr=DropBox_Transact(Tempstr, FS, "/users/get_current_account", "");
    if (StrValid(Tempstr))
    {
        P=ParserParseDocument("json", Tempstr);
        if (P)
        {
            SetVar(FS->Vars, "account_id", ParserGetValue(P, "/account_id"));
            SetVar(FS->Vars, "display_name", ParserGetValue(P, "/name/display_name"));
            SetVar(FS->Vars, "account_type", ParserGetValue(P, "/account_type/.tag"));
            RetVal=TRUE;
            ParserItemsDestroy(P);
        }
    }


    Destroy(Tempstr);
    return(RetVal);
}



static char *DropBox_GetDiskQuota(char *RetStr, TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    PARSER *P;
    float total, used;

    Tempstr=DropBox_Transact(Tempstr, FS, "/users/get_space_usage", "");
    P=ParserParseDocument("json", Tempstr);
    if (P)
    {
        total=atof(ParserGetValue(P, "/allocation/allocated"));
        used=atof(ParserGetValue(P, "/used"));
        RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f", total, total-used, used);
        ParserItemsDestroy(P);
    }

    Destroy(Tempstr);
    return(RetStr);
}


static char *DropBox_GetSharedLink(char *RetStr, TFileStore *FS, const char *Path)
{
    char *URL=NULL, *Tempstr=NULL;
    PARSER *P;
    float total, used;

    Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path, "\",\"settings\": {\"audience\": \"public\",\"access\": \"viewer\",\"requested_visibility\": \"public\",\"allow_download\": true}}", NULL);

    URL=DropBox_Transact(URL, FS, "/sharing/create_shared_link_with_settings", Tempstr);
    P=ParserParseDocument("json", URL);
    if (P)
    {
        RetStr=CopyStr(RetStr, ParserGetValue(P, "/url"));
        ParserItemsDestroy(P);
    }

    Destroy(Tempstr);
    Destroy(URL);
    return(RetStr);
}

static char *DropBox_GetHash(char *RetStr, TFileStore *FS, const char *Path)
{
    TFileItem *FI;
    const char *ptr;

    FI=FileStoreGetFileInfo(FS, Path);
    if (FI)
    {
        if (StrValid(FI->hash))
        {
            ptr=strchr(FI->hash, ':');
            if (ptr) RetStr=CopyStr(RetStr, ptr+1);
        }
    }

    return(RetStr);
}


static char *DropBox_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=CopyStr(RetStr, "");

    if (StrValid(ValName))
    {
        if (strcasecmp(ValName, "DiskQuota")==0) RetStr=DropBox_GetDiskQuota(RetStr, FS, Path);
        if (strcasecmp(ValName, "ShareLink")==0) RetStr=DropBox_GetSharedLink(RetStr, FS, Path);
        if (strcasecmp(ValName, "dropboxhash")==0) RetStr=DropBox_GetHash(RetStr, FS, Path);
    }

    return(RetStr);
}


static OAUTH *DropBox_OAuth(TFileStore *FS, OAUTH *Ctx, int Force)
{
    char *Tempstr=NULL;
    int result;

    if (Force || (! OAuthLoad(Ctx, FS->URL, "")) )
    {
        SetVar(Ctx->Vars, "redirect_uri", ""); //no redirect_uri used in dropbox authentication
        SetVar(Ctx->Vars, "token_access_type", "offline");
        OAuthStage1(Ctx, "https://www.dropbox.com/oauth2/authorize");
        printf("GOTO: %s in a browser\n",Ctx->VerifyURL);
        printf("Login and/or grant access, then cut and past the access code back to this program.\n\nAccess Code: ");
        fflush(NULL);

        Tempstr=SetStrLen(Tempstr,1024);
        result=read(0,Tempstr,1024);
        if (result > 0)
        {
            StripTrailingWhitespace(Tempstr);
            StripTrailingWhitespace(Tempstr);
            SetVar(Ctx->Vars, "code", Tempstr);
            OAuthFinalize(Ctx, "https://api.dropbox.com/oauth2/token");
        }
    }

    if (Settings->Flags & (SETTING_DEBUG | SETTING_VERBOSE)) printf("AccessToken: %s RefreshToken: %s\n",Ctx->AccessToken, Ctx->RefreshToken);
    FS->Pass=CopyStr(FS->Pass, Ctx->AccessToken);

//FS->Settings |= FS_OAUTH;
//ConfigFileSaveFileStore(FS);

    Destroy(Tempstr);

    return(Ctx);
}



static int DropBox_Connect(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    char *ptr;
    int RetVal=FALSE;
    STREAM *S;
    OAUTH *OauthCtx;


    if (! SSLAvailable())
    {
        UI_Output(UI_OUTPUT_ERROR, "SSL/TLS support not compiled in");
        return(FALSE);
    }


    OauthCtx=OAuthCreate("pkce", FS->URL, DROPBOX_CLIENT_ID, "", "", "https://www.dropbox.com/oauth2/token");
    DropBox_OAuth(FS, OauthCtx, FALSE);
    RetVal=DropBox_Info(FS);
    if (RetVal != TRUE)
    {
        OauthCtx->AccessToken=CopyStr(OauthCtx->AccessToken, "");
        if (OAuthRefresh(OauthCtx, "https://www.dropbox.com/oauth2/token?grant_type=refresh_token&refresh_token=$(refresh_token)&client_id=$(client_id)"))
        {
            FS->Pass=CopyStr(FS->Pass, OauthCtx->AccessToken);
            if (StrValid(OauthCtx->AccessToken)) OAuthSave(OauthCtx, "");
        }
        else DropBox_OAuth(FS, OauthCtx, TRUE);

        RetVal=DropBox_Info(FS);
    }


    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetVal);
}


int DropBox_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS | FILESTORE_SHARELINK | FILESTORE_USAGE;

    FS->Connect=DropBox_Connect;
    FS->ListDir=DropBox_ListDir;
    FS->OpenFile=DropBox_OpenFile;
    FS->CloseFile=DropBox_CloseFile;
    FS->ReadBytes=DropBox_ReadBytes;
    FS->WriteBytes=DropBox_WriteBytes;
    FS->UnlinkPath=DropBox_Unlink;
    FS->RenamePath=DropBox_Rename;
    FS->GetValue=DropBox_GetValue;
    FS->MkDir=DropBox_MkDir;

    AppendVar(FS->Vars, "HashTypes", "dropboxhash");

    return(TRUE);
}
