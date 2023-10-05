#include "dropbox.h"
#include "errors_and_logging.h"

#define DROPBOX_CLIENT_ID "tachgefepatxzya"

char *DropBox_Transact(char *RetStr, TFileStore *FS, const char *Path, const char *Args)
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
        Tempstr=MCopyStr(Tempstr, "$(filestore) GET ", URL, NULL);
    }
    else
    {
        ConnectConfig=FormatStr(ConnectConfig, "w Authorization='Bearer %s'", FS->Pass);
        Tempstr=MCopyStr(Tempstr, "$(filestore): POST ", URL, NULL);
    }

    HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, "", "");
    S=STREAMOpen(URL, ConnectConfig);
    if (S)
    {
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
            //printf("%s\n", Tempstr);
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


STREAM *DropBox_OpenFile(TFileStore *FS, const char *Path, const char *OpenFlags, uint64_t Size)
{
    STREAM *S;
    char *URL=NULL, *Tempstr=NULL;

    if (StrValid(OpenFlags) && (*OpenFlags=='w'))
    {
        URL=MCopyStr(URL, "https://content.dropboxapi.com/2/files/upload");
        Tempstr=FormatStr(Tempstr, "w Authorization='Bearer %s' 'Dropbox-API-Arg={\"path\": \"%s\",\"mode\": \"add\",\"autorename\": true,\"mute\": false,\"strict_conflict\": false}' Content-Type=application/octet-stream Content-Length=%lld", FS->Pass, Path, Size);
        S=STREAMOpen(URL, Tempstr);
    }
    else
    {
        URL=MCopyStr(URL, "https://content.dropboxapi.com/2/files/download");
        Tempstr=FormatStr(Tempstr, "r Authorization='Bearer %s' 'Dropbox-API-Arg={\"path\": \"%s\"}'", FS->Pass, Path);
        fprintf(stderr, "%s\n", Tempstr);
        S=STREAMOpen(URL, Tempstr);
    }

    if (! S) HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, URL, "");

    Destroy(Tempstr);
    Destroy(URL);

    return(S);
}


int DropBox_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL;

    STREAMWriteLine("\r\n", S);
    STREAMCommit(S);
    Tempstr=STREAMReadDocument(Tempstr, S);
    printf("%s\n", Tempstr);
    STREAMClose(S);

    Destroy(Tempstr);

    return(TRUE);
}


int DropBox_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}

int DropBox_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}


int DropBox_Unlink(TFileStore *FS, const char *Path)
{
    STREAM *S;
    char *JSON=NULL, *Tempstr=NULL;
    int RetVal=FALSE;


    Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path,"\"}", NULL);
    JSON=DropBox_Transact(JSON, FS, "/files/delete_v2", Tempstr);
    if (StrValid(JSON)) RetVal=TRUE;

    printf("%s\n", JSON);
    Destroy(Tempstr);
    Destroy(JSON);

    return(RetVal);
}


int DropBox_Rename(TFileStore *FS, const char *OldPath, const char *NewPath)
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


int DropBox_MkDir(TFileStore *FS, const char *Path, int Mkdir)
{
    STREAM *S;
    char *JSON=NULL, *Tempstr=NULL;
    int RetVal=FALSE;

    Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path,"\", \"autorename\": false}", NULL);
    JSON=DropBox_Transact(JSON, FS, "/files/create_folder_v2", Tempstr);
    if (StrValid(JSON)) RetVal=TRUE;

    printf("%s\n", JSON);
    Destroy(Tempstr);
    Destroy(JSON);

    return(RetVal);
}


ListNode *DropBox_ListDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *JSON=NULL;
    ListNode *Items, *FilesP, *Curr;
    TFileItem *FI;
    PARSER *P;
    uint64_t size;

//{\"path\": \"/Homework/math\",\"recursive\": false,\"include_media_info\": false,\"include_deleted\": false,\"include_has_explicit_shared_members\": false,\"include_mounted_folders\": true,\"include_non_downloadable_files\": true}
//
//{"entries": [{".tag": "file", "name": "Get Started with Dropbox Paper.url", "path_lower": "/get started with dropbox paper.url", "path_display": "/Get Started with Dropbox Paper.url", "id": "id:NY2cP0crwoAAAAAAAAAABg", "client_modified": "2020-04-08T22:34:21Z", "server_modified": "2020-04-08T22:34:21Z", "rev": "015a2cf1a0002b800000001bb1c96b0", "size": 240, "is_downloadable": true, "content_hash": "f40c1228343d7e2a632281c986dbb7af3491b9b63ddfd0eb10fee2c913f6cfa7"}, {".tag": "file", "name": "Get Started with Dropbox.pdf", "path_lower": "/get started with dropbox.pdf", "path_display": "/Get Started with Dropbox.pdf", "id": "id:NY2cP0crwoAAAAAAAAAABw", "client_modified": "2020-04-08T22:34:21Z", "server_modified": "2020-04-08T22:34:21Z", "rev": "015a2cf1a0002b900000001bb1c96b0", "size": 1102331, "is_downloadable": true, "content_hash": "f7ad488deb7d81790340ecd676fe6e47f0a6064fb99b982685b752d58611c1cb"}], "cursor": "AAGCmtkBfAAxfYBp8WQUxW5pxUiT8YqEgBZkX5XJcUPP9Jn30T1dEKOkZtTKXzlUHPqNnKmEkXy9CrRDZmfHrnwyl68rwi6oEHVcwa4l8-BT_nHsZ6chH00bnqOZtgWymatxb_Lyh3EL4JlJ_ENLOYje6Y3OWGqOmE3DP_RfbV1Z-KBFLUqO2ARoQi9MfavtEi8", "has_more": false}


    Items=ListCreate();

    if (strcmp(Path, "/")==0) Tempstr=CopyStr(Tempstr, "{\"path\": \"\",\"recursive\": false}");
    else Tempstr=MCopyStr(Tempstr, "{\"path\": \"", Path,"\",\"recursive\": false}", NULL);

    JSON=DropBox_Transact(JSON, FS, "/files/list_folder", Tempstr);
    P=ParserParseDocument("json", JSON);
    if (P)
    {
        Curr=ParserOpenItem(P, "entries");
        while (Curr)
        {
            if (StrValid(ParserGetValue(Curr, "name")))
            {
                size=(uint64_t) strtoll(ParserGetValue(Curr, "size"), NULL, 10);
                FI=FileItemCreate(ParserGetValue(Curr, "name"), FTYPE_FILE, size, 0666);
                FI->path=CopyStr(FI->path, ParserGetValue(Curr, "path_lower"));
                FI->mtime=DateStrToSecs("%Y-%m-%dT%H:%M:%S.", ParserGetValue(Curr,"client_modified"), NULL);

                ListAddNamedItem(Items, FI->name, FI);
            }
            Curr=ListGetNext(Curr);
        }
    }

    return(Items);
}






/*
 * {"account_id": "dbid:AAATkllVLE8-FjpRB3Wz4ALOPO7azZ9pHGU", "name": {"given_name": "Colum", "surname": "Paget", "familiar_name": "Colum", "display_name": "Colum Paget", "abbreviated_name": "CP"}, "email": "colum.paget@gmail.com", "email_verified": true, "disabled": false, "country": "GB", "locale": "en", "referral_link": "https://www.dropbox.com/referrals/AABhVoK3mS6-Jnaeg0pYZyt6NxPh_oaQFqk?src=app9-11426688", "is_paired": false, "account_type": {".tag": "basic"}, "root_info": {".tag": "user", "root_namespace_id": "7434180272", "home_namespace_id": "7434180272"}}

 {"used": 1102571, "allocation": {".tag": "individual", "allocated": 2147483648}}

*/

int DropBox_Info(TFileStore *FS)
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



char *DropBox_GetDiskQuota(char *RetStr, TFileStore *FS, const char *Path)
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


char *DropBox_GetSharedLink(char *RetStr, TFileStore *FS, const char *Path)
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


char *DropBox_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=CopyStr(RetStr, "");
    if (strcmp(ValName, "DiskQuota")==0) RetStr=DropBox_GetDiskQuota(RetStr, FS, Path);
    if (strcmp(ValName, "ShareLink")==0) RetStr=DropBox_GetSharedLink(RetStr, FS, Path);
    return(RetStr);
}


static OAUTH *DropBox_OAuth(TFileStore *FS, OAUTH *Ctx, int Force)
{
    char *Tempstr=NULL;

    if (Force || (! OAuthLoad(Ctx, FS->URL, "")) )
    {
        SetVar(Ctx->Vars, "redirect_uri", ""); //no redirect_uri used in dropbox authentication
        SetVar(Ctx->Vars, "token_access_type", "offline");
        OAuthStage1(Ctx, "https://www.dropbox.com/oauth2/authorize");
        printf("GOTO: %s in a browser\n",Ctx->VerifyURL);
        printf("Login and/or grant access, then cut and past the access code back to this program.\n\nAccess Code: ");
        fflush(NULL);

        Tempstr=SetStrLen(Tempstr,1024);
        read(0,Tempstr,1024);
        StripTrailingWhitespace(Tempstr);
        StripTrailingWhitespace(Tempstr);
        SetVar(Ctx->Vars, "code", Tempstr);
        OAuthFinalize(Ctx, "https://api.dropbox.com/oauth2/token");
    }

    printf("AccessToken: %s RefreshToken: %s\n",Ctx->AccessToken, Ctx->RefreshToken);
    FS->Pass=CopyStr(FS->Pass, Ctx->AccessToken);

//FS->Settings |= FS_OAUTH;
//ConfigFileSaveFileStore(FS);

    Destroy(Tempstr);

    return(Ctx);
}



int DropBox_Connect(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    char *ptr;
    int RetVal=FALSE;
    STREAM *S;
    OAUTH *OauthCtx;


    if (! SSLAvailable())
    {
        printf("ERROR: SSL/TLS support not compiled in\n");
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


    /*
        FS->FileInfo=FTP_FileInfo;
        FS->ChDir=FTP_ChDir;
    */

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

    return(TRUE);
}
