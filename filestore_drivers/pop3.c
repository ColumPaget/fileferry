#include "pop3.h"
#include "inet_protocols.h"
#include "../errors_and_logging.h"
#include "../ui.h"
#include "../password.h"

static void POP3_ReadFeatures(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL, *Token=NULL, *FeatureList=NULL;
    const char *ptr;

    STREAMWriteLine("CAPA\r\n",FS->S);
    STREAMFlush(FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,&Verbiage, INET_OKAY))
    {
        //First line will be 'Features follow' or something like that
        ptr=GetToken(Verbiage,"\n",&Token,0);
        ptr=GetToken(ptr,"\n",&Token,0);
        while (ptr)
        {
            StripLeadingWhitespace(Token);
            StripTrailingWhitespace(Token);
            if (StrValid(Token))
            {
                if (strcmp(Token, ".")==0) break;
                if (StrValid(FeatureList)) FeatureList=MCatStr(FeatureList,", ",Token,NULL);
                else FeatureList=CatStr(FeatureList,Token);
            }
            ptr=GetToken(ptr,"\n",&Token,0);
        }
        SetVar(FS->Vars,"Features",FeatureList);
    }


    DestroyString(FeatureList);
    DestroyString(Verbiage);
    DestroyString(Tempstr);
    DestroyString(Token);
}


static int POP3_Login(TFileStore *FS, int Try)
{
    char *Tempstr=NULL, *Verbiage=NULL, *Token=NULL, *LoginBanner=NULL;
    const char *ptr, *p_Pass;
    int result=FALSE, Flags=0, len;

    if (! FS) return(FALSE);
    if (! StrValid(FS->User)) return(FALSE);

//  STREAMSetTimeout(FS->S,3000);
// InetReadResponse(FS->S, FS, &Tempstr,&Verbiage, INET_OKAY);

    Tempstr=MCopyStr(Tempstr,"USER ",FS->User,"\r\n",NULL);
    STREAMWriteLine(Tempstr,FS->S);


    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY))
    {
        len=PasswordGet(FS, Try, &p_Pass);
        Tempstr=CopyStr(Tempstr, "PASS ");
        Tempstr=CatStrLen(Tempstr, p_Pass, len);
        Tempstr=CatStr(Tempstr, "\r\n");
        STREAMWriteLine(Tempstr, FS->S);
        if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY)) result=TRUE;
    }

    DestroyString(Tempstr);
    DestroyString(Verbiage);
    DestroyString(LoginBanner);
    DestroyString(Token);

    return(result);
}

static int POP3_UnlinkPath(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    int RetVal=FALSE;
    const char *ptr;

    ptr=Path;
    if (*ptr=='/') ptr++;
    Tempstr=MCopyStr(Tempstr, "DELE ", ptr, "\r\n", NULL);
    STREAMWriteLine(Tempstr, FS->S);
    STREAMFlush(FS->S);
    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY)) RetVal=TRUE;
    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetVal);
}

static STREAM *POP3_OpenFile(TFileStore *FS, const char *Path, int OpenMode, uint64_t Offset, uint64_t Size)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    STREAM *S=NULL;
    const char *ptr;

    ptr=Path;
    if (*ptr=='/') ptr++;
    Tempstr=MCopyStr(Tempstr, "RETR ", ptr, "\r\n", NULL);
    STREAMWriteLine(Tempstr, FS->S);
    STREAMFlush(FS->S);
    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY)) S=FS->S;
    Destroy(Tempstr);
    Destroy(Verbiage);
    return(S);
}

static int POP3_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t Offset, uint32_t Len)
{
    char *Tempstr=NULL;
    int len=0;

    Tempstr=STREAMReadLine(Tempstr, S);
    if (! Tempstr) return(-1);
    if (strcmp(Tempstr, ".\r\n")==0) return(-1);
    if (StrLen(Tempstr) <= Len)
    {
        strncpy(Buffer, Tempstr, Len);
        len=StrLen(Buffer);
    }

    Destroy(Tempstr);
    return(len);
}


static int POP3_CloseFile(TFileStore *FS, STREAM *S)
{
//don't close anything! File is sent on the control channel!
    return(TRUE);
}



static TFileItem *POP3_ListDirReadItem(TFileStore *FS, int ItemNo)
{
    char *Tempstr=NULL, *Data=NULL, *Token=NULL;
    int InHeaders;
    TFileItem *FI=NULL;
    const char *ptr;

    FI=FileItemCreate("", FTYPE_FILE, 0, 0);

    FI->path=FormatStr(FI->path, "%d", ItemNo);
    Tempstr=FormatStr(Tempstr, "LIST %d\r\n", ItemNo);
    STREAMWriteLine(Tempstr, FS->S);
    STREAMFlush(FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr, &Data, INET_OKAY))
    {
        ptr=GetToken(Data, " ", &Token, 0);
        ptr=GetToken(ptr, " ", &Token, 0);
        FI->size=atoi(Token);
    }

    Tempstr=FormatStr(Tempstr, "TOP %d 0\r\n", ItemNo);
    STREAMWriteLine(Tempstr, FS->S);
    STREAMFlush(FS->S);
    if (InetReadResponse(FS->S, FS, &Tempstr, &Data, INET_OKAY))
    {
        Tempstr=STREAMReadLine(Tempstr, FS->S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            if (strncasecmp(Tempstr, "Subject:", 8)==0) FI->name=CopyStr(FI->name, Tempstr+8);
            if (strncasecmp(Tempstr, "From:", 5)==0) FI->user=CopyStr(FI->user, Tempstr+5);
            if (strncasecmp(Tempstr, "Date:", 5)==0)
            {
                ptr=Tempstr+5;
                while (isspace(*ptr)) ptr++;
                FI->mtime=DateStrToSecs("%a, %d %b %Y %H:%M:%S", ptr, NULL);
                FI->ctime=FI->mtime;
                FI->atime=FI->mtime;
            }
            if (strcmp(Tempstr, ".")==0) break;
            Tempstr=STREAMReadLine(Tempstr, FS->S);
        }
    }

    StripLeadingWhitespace(FI->name);
    StripLeadingWhitespace(FI->user);

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Data);

    return(FI);
}


static ListNode *POP3_ListDir(TFileStore *FS, const char *Path)
{
    int result, MLSD=FALSE;
    char *Tempstr=NULL, *Data=NULL;
    ListNode *Files=NULL;
    TFileItem *FI=NULL;
    int FCount, i;

    Files=ListCreate();
    SendLoggedLine("STAT\r\n", FS, FS->S);
    if (InetReadResponse(FS->S, FS, &Tempstr, &Data, INET_OKAY))
    {
        FCount=strtol(Data, NULL, 10);
        for (i=0; i < FCount; i++)
        {
            FI=POP3_ListDirReadItem(FS, i+1);
            ListAddNamedItem(Files, FI->name, FI);
        }
    }

    Destroy(Tempstr);
    Destroy(Data);

    return(Files);
}


static int POP3_Connect(TFileStore *FS)
{
    char *Proto=NULL, *Host=NULL, *PortStr=NULL, *Path=NULL, *Tempstr=NULL, *Verbiage=NULL;
    char *ptr;
    int RetVal=FALSE, i;


    ParseURL(FS->URL, &Proto, &Host, &PortStr, NULL, NULL, &Path,  NULL);
    if (! StrValid(PortStr))
    {
        if (strcasecmp(Proto, "pop3s")==0) PortStr=CopyStr(PortStr, "995");
        else PortStr=CopyStr(PortStr, "110");
    }

    if (strcasecmp(Proto, "pop3s")==0)
    {
        Tempstr=MCopyStr(Tempstr, "tls:", Host, ":", PortStr, NULL);
        FS->Flags |= FILESTORE_TLS;
    }
    else Tempstr=MCopyStr(Tempstr, "tcp:", Host, ":", PortStr, NULL);

    FS->S=STREAMOpen(Tempstr, "");
    if (FS->S)
    {
        RetVal=TRUE;

        STREAMSetTimeout(FS->S, 3000);
        InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY);
        if (StrLen(Verbiage) > 4) SetVar(FS->Vars,"ServerBanner",Verbiage);


        if (StrValid(Tempstr) && (strcmp(Tempstr, "+OK")==0) )
        {
            //if (strcasecmp(Proto, "ftps")==0) FTP_AuthTLS(FS);

            for (i=0; i < 5; i++)
            {
                RetVal=POP3_Login(FS, i);
                if (RetVal) break;
            }
        }
        else SetVar(FS->Vars,"ServerError",Verbiage);
    }

    if (RetVal)
    {
        if (FS->Flags & FILESTORE_TLS) FileStoreRecordCipherDetails(FS, FS->S);
        //POP3_ReadFeatures(FS);
    }
    else
    {
        STREAMClose(FS->S);
        FS->S=NULL;
    }


    Destroy(Proto);
    Destroy(Host);
    Destroy(Path);
    Destroy(PortStr);
    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetVal);

}

static int POP3_DisConnect(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;

    STREAMWriteLine("QUIT\r\n", FS->S);
    STREAMFlush(FS->S);
    InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY);

    Destroy(Tempstr);
    Destroy(Verbiage);
}

int POP3_Attach(TFileStore *FS)
{
    FS->Connect=POP3_Connect;
    FS->DisConnect=POP3_DisConnect;
    FS->ListDir=POP3_ListDir;
    FS->OpenFile=POP3_OpenFile;
    FS->CloseFile=POP3_CloseFile;
    FS->ReadBytes=POP3_ReadBytes;
    FS->UnlinkPath=POP3_UnlinkPath;

    return(TRUE);
}
