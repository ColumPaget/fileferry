#include "filesanywhere.h"
#include "http.h"
#include "../fileitem.h"
#include "../ui.h"

#define LOGIN_URL "http://api.filesanywhere.com/AccountLogin"
#define API_KEY "40D380BA-4E80-46D9-8629-6A2F229DBA94"


static char *FilesAnywhere_Path(char *FAWPath, const char *Path)
{
    FAWPath=CopyStr(FAWPath, Path);
    strrep(FAWPath, '/', '\\');

    return(FAWPath);
}


static int FilesAnywhere_Command(TFileStore *FS, const char *XML, const char *SOAPAction, char **ResponseData)
{
    char *Tempstr=NULL, *PostData=NULL;
    const char *ptr;
    int RetVal=FALSE;
    STREAM *S;

    if (! FS) return(FALSE);

    PostData=MCopyStr(PostData,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">\n<soap:Body>\n", XML, "</soap:Body>\n</soap:Envelope>\n", NULL);
    Tempstr=FormatStr(Tempstr, "w SOAPAction=%s Content-Type='text/xml; charset=utf-8' Content-Length=%d", SOAPAction, StrLen(PostData));

    //printf("XML: %s\n", XML);
    S=STREAMOpen("https://api.filesanywhere.com/v2/fawapi.asmx", Tempstr);
    if (S)
    { 
        FS->Flags |= FILESTORE_TLS;
        FileStoreRecordCipherDetails(FS, S);

        STREAMWriteLine(PostData, S);
        STREAMCommit(S);
        RetVal=HTTP_CheckResponseCode(S);
        *ResponseData=STREAMReadDocument(*ResponseData, S);
        if (Settings->Flags & (SETTING_DEBUG | SETTING_VERBOSE)) fprintf(stderr, "\n%s\n", *ResponseData);
        STREAMClose(S);
    }

    DestroyString(Tempstr);
    DestroyString(PostData);

    return(RetVal);
}




static TFileItem *FilesAnywhere_FileInfo(TFileStore *FS, const char *Path)
{
    TFileItem *Item;

    return(Item);
}


static TFileItem *FilesAnywhere_ReadFileEntry(const char **XML)
{
    char *Token=NULL, *TagName=NULL, *TagData=NULL;
    const char *ptr;
    const char *Tags[]= {"Type","Name","Size","Path","DateLastModified","/ItemWithPermissions",NULL};
    typedef enum {FAW_TYPE,FAW_NAME,FAW_SIZE,FAW_PATH,FAW_MTIME,FAW_END} EFAW;
    TFileItem *FI=NULL;
    int result;

    FI=FileItemCreate("", 0, 0, 0);
    *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
    while (*XML)
    {
        result=MatchTokenFromList(TagName,Tags,0);
        if (result==FAW_END) break;
        switch (result)
        {
        case FAW_NAME:
            *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
            FI->name=CopyStr(FI->name,TagData);
            break;

        case FAW_PATH:
            *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
            //FI->path=CopyStr(FI->path,TagData);
            //strrep(FI->Path,'\\','/');
            break;


        case FAW_MTIME:
            *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
            FI->mtime=DateStrToSecs("%m/%d/%Y %H:%M:%S",TagData,NULL);
            break;

        case FAW_SIZE:
            *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
            FI->size=atoi(TagData);
            break;

        case FAW_TYPE:
            *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
            if (strcasecmp(TagData,"Folder")==0) FI->type=FTYPE_DIR;
            else FI->type=FTYPE_FILE;
            break;
        }

        *XML=XMLGetTag(*XML,NULL,&TagName,&TagData);
    }

    FI->path=CopyStr(FI->path,FI->name);

    DestroyString(TagName);
    DestroyString(TagData);
    DestroyString(Token);

    return(FI);
}



static ListNode *FilesAnywhere_ListDir(TFileStore *FS, const char *Path)
{
    ListNode *Files=NULL;
    char *Tempstr=NULL, *XML=NULL;
    char *TagName=NULL, *TagData=NULL;
    const char *ptr=NULL;
    TFileItem *FI;
    int result=FALSE;

    XML=MCopyStr(XML,"<ListItemsWithPermissions xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<Path>",FS->CurrDir,"</Path><PageSize>200</PageSize><PageNum>0</PageNum>\n", "</ListItemsWithPermissions>\n", NULL);

    if (FilesAnywhere_Command(FS, XML, "http://api.filesanywhere.com/ListItemsWithPermissions",&Tempstr)==TRUE)
    {
        Files=ListCreate();
        ptr=XMLGetTag(Tempstr,NULL,&TagName,&TagData);
        while (ptr)
        {
            if (strcasecmp(TagName,"ItemWithPermissions")==0)
            {
                FI=FilesAnywhere_ReadFileEntry(&ptr);
                ListAddNamedItem(Files,FI->name,FI);
            }
            ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
        }
    }

    DestroyString(Tempstr);
    DestroyString(XML);

    return(Files);
}



static int FilesAnywhere_MkDir(TFileStore *FS, const char *Dir, int Mode)
{
    char *Tempstr=NULL, *XML=NULL;
    int result=FALSE;

    if (! FS) return(FALSE);

    XML=MCopyStr(XML, "<CreateFolder xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<Path>",FS->CurrDir,"</Path>\n", "<NewFolderName>",Dir,"</NewFolderName>\n", "</CreateFolder>",NULL);
    result=FilesAnywhere_Command(FS, XML, "https://api.filesanywhere.com/CreateFolder",&Tempstr);

    DestroyString(Tempstr);
    DestroyString(XML);

    if (result==TRUE) return(TRUE);
    return(FALSE);
}



static int FilesAnywhere_Symlink(TFileStore *FS, char *FromPath, char *ToPath)
{
    int result=FALSE;
    char *Tempstr=NULL, *Verbiage=NULL;


    Destroy(Verbiage);
    Destroy(Tempstr);

    return(result);
}


static int FilesAnywhere_DeleteItem(TFileStore *FS, const char *Path, int Type, char **RetStr)
{
    char *XML=NULL, *FAWPath=NULL;
    int result;

    FAWPath=FilesAnywhere_Path(FAWPath, Path);
    XML=MCopyStr(XML, "<DeleteItems xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n",NULL);
    if (Type==FTYPE_DIR) XML=MCatStr(XML,"<ItemsToDelete> <Item> <Type>folder</Type> <Path>",FAWPath,"</Path> </Item>\n","</ItemsToDelete> </DeleteItems>\n",NULL);
    else XML=MCatStr(XML,"<ItemsToDelete> <Item> <Type>file</Type> <Path>",FAWPath,"</Path> </Item>\n","</ItemsToDelete> </DeleteItems>\n",NULL);

    result=FilesAnywhere_Command(FS, XML, "http://api.filesanywhere.com/DeleteItems", RetStr);

    Destroy(FAWPath);
    Destroy(XML);
    return(result);
}


static int FilesAnywhere_Unlink(TFileStore *FS, const char *Path)
{
    int result=FALSE, RetVal=FALSE;
    char *Tempstr=NULL, *XML=NULL;
    char *TagData=NULL, *TagName=NULL;
    const char *ptr;

    if (! FS) return(FALSE);

    result=FilesAnywhere_DeleteItem(FS, Path, FTYPE_FILE, &Tempstr);
    result=FilesAnywhere_DeleteItem(FS, Path, FTYPE_DIR, &Tempstr);

    if (result==TRUE)
    {
        ptr=XMLGetTag(Tempstr,NULL,&TagName,&TagData);
        while (ptr)
        {
            if (strcasecmp(TagName,"deleted")==0)
            {
                ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
                if (strcmp(TagData,"true")==0) RetVal=TRUE;
            }
            ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
        }
    }

    DestroyString(TagName);
    DestroyString(TagData);
    DestroyString(Tempstr);
    DestroyString(XML);

    return(RetVal);
}




static int FilesAnywhere_Rename(TFileStore *FS, const char *FromPath, const char *ToPath)
{
    char *Tempstr=NULL, *XML=NULL;
    char *QuotedTo=NULL, *QuotedFrom=NULL;
    char *TagName=NULL, *TagData=NULL;
    const char *ptr, *dptr;
    int result=FALSE, RetVal=FALSE;

    QuotedFrom=FilesAnywhere_Path(QuotedFrom, FromPath);
    ptr=strrchr(QuotedFrom,'\\');

    QuotedTo=FilesAnywhere_Path(QuotedTo, ToPath);
    dptr=strrchr(QuotedTo,'\\');

    if (dptr && ptr)
    {
        if ((*(dptr+1)=='\0') || (strncmp(QuotedTo, QuotedFrom, dptr - QuotedTo) != 0))
        {
            XML=MCopyStr(XML, "<MoveItems xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<SrcPath>",QuotedFrom,"</SrcPath>\n", "<DstPath>",QuotedTo,"</DstPath>\n","</MoveItems>\n",NULL);
            Tempstr=CopyStr(Tempstr, "http://api.filesanywhere.com/MoveItems");
        }
    }

    if (! StrLen(Tempstr))
    {
        //Full path not allowed when changing file name

        XML=MCopyStr(XML, "<RenameItem xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<Path>",QuotedFrom,"</Path>\n", "<Type>","file","</Type>\n", "<NewName>",QuotedTo,"</NewName>\n","</RenameItem>\n",NULL);
        Tempstr=CopyStr(Tempstr, "http://api.filesanywhere.com/RenameItem");
    }

    if (FilesAnywhere_Command(FS, XML, Tempstr, &Tempstr)==TRUE)
    {
        ptr=XMLGetTag(Tempstr,NULL,&TagName,&TagData);
        while (ptr)
        {
            if (
                (strcasecmp(TagName,"Renamed")==0) ||
                (strcasecmp(TagName,"Moved")==0)
            )
            {
                ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
                if (strcmp(TagData,"true")==0) RetVal=TRUE;
            }
            ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
        }
    }

    DestroyString(QuotedTo);
    DestroyString(QuotedFrom);
    DestroyString(Tempstr);
    DestroyString(TagName);
    DestroyString(TagData);
    DestroyString(XML);

    return(RetVal);
}




static int FilesAnywhere_ChMod(TFileStore *FS, const char *Path, int Mode)
{
    char *Tempstr=NULL;
    int result=FALSE;

    DestroyString(Tempstr);

    return(result);
}


static int FilesAnywhere_ChPassword(TFileStore *FS, const char *Old, const char *New)
{
    char *Tempstr=NULL;
    int result=FALSE;


    DestroyString(Tempstr);

    return(result);
}





static STREAM *FilesAnywhere_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    char *Tempstr=NULL, *URL=NULL, *XML=NULL;
    char *TagName=NULL, *TagData=NULL;
    const char *ptr;
    STREAM *S=NULL;
    int result;


    if (OpenFlags & XFER_FLAG_WRITE)
    {
        S=STREAMCreate();
        STREAMSetValue(S, "transfer_filename", Path);
        STREAMSetValue(S, "transfer_filechunkoffset", "0");
        Tempstr=FormatStr(Tempstr,"%d",Size);
        STREAMSetValue(S, "transfer_filesize", Tempstr);

        //Actual transaction happens in FAWWriteBytes
    }
    else
    {
        /*
          if (Flags & OPEN_LOCK)
          {
            TagData=CopyStr(TagData,"<Comments>CheckedOut</Comments>\n");
            XML=MCopyStr(XML, "<DownloadWithCheckout xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<ItemPath>",Path,"</ItemPath>\n",TagData, "</DownloadWithCheckout>", NULL);
            Tempstr=CopyStr(Tempstr, "http://api.filesanywhere.com/DownloadWithCheckout");
          }
          else
        */
        {
            XML=MCopyStr(XML, "<DownloadFile xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<Path>",Path,"</Path>\n","</DownloadFile>", NULL);
            Tempstr=CopyStr(Tempstr, "http://api.filesanywhere.com/DownloadFile");
        }

        if ( FilesAnywhere_Command(FS, XML, Tempstr, &Tempstr)==TRUE)
        {
            ptr=XMLGetTag(Tempstr,NULL,&TagName,&TagData);
            while (ptr)
            {
                if (strcasecmp(TagName,"URL")==0)
                {
                    ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
                    URL=CopyStr(URL,TagData);
                }
                ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
            }
            if (StrLen(URL))
            {
                S=STREAMOpen(URL, "r");
                if (HTTP_CheckResponseCode(S) != TRUE)
                {
                    STREAMClose(S);
                    S=NULL;
                }
            }
        }
    }


    DestroyString(Tempstr);
    DestroyString(URL);
    DestroyString(TagName);
    DestroyString(TagData);
    DestroyString(XML);

    return(S);
}


static int FilesAnywhere_CloseFile(TFileStore *FS, STREAM *S)
{
    if (S) STREAMClose(S);

    return(TRUE);
}



static int FilesAnywhere_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


static int FilesAnywhere_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset,  uint32_t Len)
{
    int result=-1, BytesSent, EncodedLen, val;
    char *Base64=NULL;
    char *Tempstr=NULL, *XML=NULL;

    if (! FS) return(FALSE);

//This is the amount that will be sent after we've finished this chunk
    BytesSent=offset+Len;

//Tempstr=MCopyStr(Tempstr,FS->CurrDir,STREAMGetValue(S,"filename"),NULL);
    Tempstr=MCopyStr(Tempstr,STREAMGetValue(S,"transfer_filename"),NULL);

    XML=MCopyStr(XML, "<AppendChunk xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", "<Path>",Tempstr,"</Path>\n",NULL);
    Base64=EncodeBytes(Base64,Buffer,Len,ENCODE_BASE64);
    EncodedLen=StrLen(Base64);

    Tempstr=FormatStr(Tempstr,"%d", offset);
    XML=MCatStr(XML,"<ChunkData>",Base64,"</ChunkData>","<Offset>",Tempstr,"</Offset>", NULL);
    Tempstr=FormatStr(Tempstr,"%d",Len);
    XML=MCatStr(XML, "<BytesRead>",Tempstr,"</BytesRead>",NULL);

    if (BytesSent >= atoi(STREAMGetValue(S,"transfer_filesize")))
    {
        XML=CatStr(XML,"<isLastChunk>true</isLastChunk>");
    }
    else  XML=CatStr(XML,"<isLastChunk>false</isLastChunk>");

    XML=CatStr(XML,"</AppendChunk>");

    result=FilesAnywhere_Command(FS, XML, "http://api.filesanywhere.com/AppendChunk", &Tempstr);

    DestroyString(Tempstr);
    DestroyString(Base64);
    DestroyString(XML);

    if (result==TRUE) return(Len);
    return(0);
}


static int FilesAnywhere_Disconnect(TFileStore *FS)
{
    char *Tempstr=NULL;
    int result;

    Destroy(Tempstr);

    return(TRUE);
}



static int FilesAnywhere_Login(TFileStore *FS)
{
    char *Tempstr=NULL, *XML=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    int RetVal=FALSE;

    XML=MCopyStr(XML, "<AccountLogin xmlns=\"http://api.filesanywhere.com/\">\n", "<APIKey>", API_KEY, "</APIKey>\n", "<OrgID>0</OrgID>\n", "<UserName>",FS->User,"</UserName>\n", "<Password>",FS->Pass,"</Password>\n",
//"<ClientEncryptParam>ENCRYPTEDNO</ClientEncryptParam>\n",
//"<AllowedIPList>string</AllowedIPList>",
                 "</AccountLogin>\n",NULL);

    if (FilesAnywhere_Command(FS, XML, LOGIN_URL, &Tempstr)==TRUE)
    {
        ptr=XMLGetTag(Tempstr,NULL,&Name,&Value);
        while (ptr)
        {
            if (strcasecmp(Name,"ErrorMessage")==0)
            {
                ptr=XMLGetTag(ptr,NULL,&Name,&Value);
                SetVar(FS->Vars,"LoginError",Value);
            }

            if (strcasecmp(Name,"Token")==0)
            {
                ptr=XMLGetTag(ptr,NULL,&Name,&Value);
                SetVar(FS->Vars,"AuthToken",Value);
                RetVal=TRUE;
            }
            ptr=XMLGetTag(ptr,NULL,&Name,&Value);
        }
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
    Destroy(XML);

    return(RetVal);
}





/*
<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <GetAccountDetailsResponse xmlns="http://api.filesanywhere.com/">
      <GetAccountDetailsResult>
        <ErrorMessage>string</ErrorMessage>
        <EmailAddress>string</EmailAddress>
        <CanChangeEmailAddress>boolean</CanChangeEmailAddress>
        <StorageAllocationInBytes>long</StorageAllocationInBytes>
        <StorageAllocationDisplay>string</StorageAllocationDisplay>
        <StorageUsageInBytes>long</StorageUsageInBytes>
        <StorageUsageDisplay>string</StorageUsageDisplay>
        <UserType>string</UserType>
        <AccountStatus>string</AccountStatus>
        <RegionID>int</RegionID>
        <RegionDetails>
          <Name>string</Name>
          <UserURL>string</UserURL>
        </RegionDetails>
      </GetAccountDetailsResult>
    </GetAccountDetailsResponse>
  </soap:Body>
</soap:Envelope>
*/

static char *FilesAnywhere_GetDiskQuota(char *RetStr, TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *XML=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    float total, avail, used;
    int RetVal=FALSE;

    XML=MCopyStr(XML,"<GetAccountDetails xmlns=\"http://api.filesanywhere.com/\">\n", "<Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n</GetAccountDetails>\n", NULL);
    if (FilesAnywhere_Command(FS, XML, "http://api.filesanywhere.com/GetAccountDetails", &Tempstr)==TRUE)
    {
        ptr=XMLGetTag(Tempstr,NULL,&Name,&Value);
        while (ptr)
        {
            if (strcasecmp(Name,"ErrorMessage")==0)
            {
                ptr=XMLGetTag(ptr,NULL,&Name,&Value);
                SetVar(FS->Vars,"Error",Value);
            }

            if (strcasecmp(Name,"StorageAllocationInBytes")==0)
            {
                ptr=XMLGetTag(ptr,NULL,&Name,&Value);
                total=atof(Value);
            }

            if (strcasecmp(Name,"StorageUsageInBytes")==0)
            {
                ptr=XMLGetTag(ptr,NULL,&Name,&Value);
                used=atof(Value);
            }

            ptr=XMLGetTag(ptr,NULL,&Name,&Value);
        }
        RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f",total, total-used, used);
    }

    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
    Destroy(XML);

    return(RetStr);
}



static char *FilesAnywhere_GetSharedLink(char *RetStr, TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *XML=NULL, *FType=NULL;
    char *TagName=NULL, *TagData=NULL;
    const char *ptr;
    int result=FALSE;

    if (! FS) return(FALSE);

//XML=MCopyStr(XML, " <SendItemsELinkWithDefaults xmlns=\"http://api.filesanywhere.com/\">\n", " <Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", " <ELinkItems><Item><Type>",FType,"</Type><Path>",FI->Path,"</Path></Item></ELinkItems>\n", " <RecipientsEmailAddresses>",ToList,"</RecipientsEmailAddresses><SendEmail>Y</SendEmail>\n", " <ElinkPassWord>",Passwd,"</ElinkPassWord>\n", " </SendItemsELinkWithDefaults>\n",NULL);
//result=FAWCommand(FS, XML, "http://api.filesanywhere.com/SendItemsELinkWithDefaults",&Tempstr);

    XML=MCopyStr(XML, " <ViewFile xmlns=\"http://api.filesanywhere.com/\">\n", " <Token>",GetVar(FS->Vars,"AuthToken"),"</Token>\n", " <Path>",Path,"</Path>\n</ViewFile>\n", NULL);
    result=FilesAnywhere_Command(FS, XML, "http://api.filesanywhere.com/ViewFile", &Tempstr);

    ptr=XMLGetTag(Tempstr,NULL,&TagName,&TagData);
    while (ptr)
    {
        if (strcasecmp(TagName,"URL")==0)
        {
            ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
            RetStr=CopyStr(RetStr, TagData);
        }
        ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
    }

    DestroyString(Tempstr);
    DestroyString(FType);
    DestroyString(XML);

    return(RetStr);
}



static char *FilesAnywhere_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=CopyStr(RetStr, "");
    if (strcmp(ValName, "DiskQuota")==0) RetStr=FilesAnywhere_GetDiskQuota(RetStr, FS, Path);
    if (strcmp(ValName, "ShareLink")==0) RetStr=FilesAnywhere_GetSharedLink(RetStr, FS, Path);
    return(RetStr);
}




static int FilesAnywhere_Connect(TFileStore *FS)
{
    int RetVal=FALSE;

    if (! SSLAvailable())
    {
        printf("ERROR: SSL/TLS support not compiled in\n");
        return(FALSE);
    }

    RetVal=FilesAnywhere_Login(FS);
    while (RetVal != TRUE)
    {
        FS->Pass=UI_AskPassword(FS->Pass);
        RetVal=FilesAnywhere_Login(FS);
    }

    FS->CurrDir=MCopyStr(FS->CurrDir,"/",FS->User,"/",NULL);

    return(RetVal);
}


int FilesAnywhere_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS | FILESTORE_SHARELINK | FILESTORE_USAGE;

    FS->Connect=FilesAnywhere_Connect;
    FS->ListDir=FilesAnywhere_ListDir;
    FS->UnlinkPath=FilesAnywhere_Unlink;
    FS->OpenFile=FilesAnywhere_OpenFile;
    FS->CloseFile=FilesAnywhere_CloseFile;
    FS->ReadBytes=FilesAnywhere_ReadBytes;
    FS->WriteBytes=FilesAnywhere_WriteBytes;
    FS->RenamePath=FilesAnywhere_Rename;
    FS->MkDir=FilesAnywhere_MkDir;
    FS->GetValue=FilesAnywhere_GetValue;


    return(TRUE);
}
