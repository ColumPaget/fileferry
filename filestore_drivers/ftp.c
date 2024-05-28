#include "localdisk.h"
#include "inet_protocols.h"
#include "ls_decode.h"
#include "../filestore.h"
#include "../fileitem.h"
#include "../ui.h"
#include "../password.h"
#include "../errors_and_logging.h"
#include "../settings.h"


//send a command and read the response
static int FTP_SendCommand(TFileStore *FS, const char *Command, char **Verbiage)
{
    char *Tempstr=NULL, *Trash=NULL;
    char **ptr;
    int result=FALSE;

    SendLoggedLine(Command, FS, FS->S);

    if (Verbiage) ptr=Verbiage;
    else ptr=&Trash;

    if (InetReadResponse(FS->S, FS, &Tempstr, ptr, INET_OKAY)) result=TRUE;
    else SetVar(FS->Vars, "Error", *ptr);

    Destroy(Tempstr);
    Destroy(Trash);

    return(result);
}


static int FTP_HasFeature(TFileStore *FS, const char *Feature)
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







static int FTP_DecodePORTStr(char *PortStr, char **Address, int *Port)
{
    char *Tempstr=NULL;
    const char *ptr;
    int count;

    for (count=0; count < StrLen(PortStr); count++) if (! isdigit(PortStr[count])) PortStr[count]=',';
    ptr=PortStr;
    for (count=0; count < 4; count++)
    {
        ptr=GetToken(ptr,",",&Tempstr,0);
        *Address=CatStr(*Address,Tempstr);
        if (count < 3) *Address=CatStr(*Address,".");
    }

    ptr=GetToken(ptr,",",&Tempstr,0);
    count=atoi(Tempstr);
    *Port=count;
    *Port=*Port << 8;

    ptr=GetToken(ptr,",",&Tempstr,0);
    count=atoi(Tempstr);
    *Port=*Port | count;

    Destroy(Tempstr);
    return(TRUE);
}




static STREAM *FTP_MakePassiveDataConnection(TFileStore *FS, const char *URL)
{
    STREAM *S=NULL;

    S=STREAMOpen(URL, "");
    if (S)
    {
        if (FS->Flags & FILESTORE_TLS) DoSSLClientNegotiation(S, 0);
        //if (FS->Settings & FS_TRANSFER_TYPES)  STREAMAddStandardDataProcessor(S,"compression","zlib","");
        //if (Settings.Flags & FLAG_VERBOSE) printf("Data Connection OKAY: %s",URL);
    }

    return(S);
}


static char *FTP_NegotiatePassiveDataConnection(char *RetStr, TFileStore *FS)
{
    char *Tempstr=NULL, *Address=NULL, *Token=NULL;
    int Port, Flags=0;
    char *ptr;
    int result;


    SendLoggedLine("PASV\r\n",FS, FS->S);

    Tempstr=ReadLoggedLine(Tempstr, FS, FS->S);

    //if (Settings.Flags & FLAG_VERBOSE) printf("%s",Tempstr);
    if (StrValid(Tempstr))
    {
        if (*Tempstr=='1') Tempstr=ReadLoggedLine(Tempstr, FS, FS->S);
        if (*Tempstr=='2')
        {
            ptr=strrchr(Tempstr,'(');
            if (ptr)
            {
                ptr++;
                GetToken(ptr, ")", &Token, 0);
                FTP_DecodePORTStr(Token, &Address, &Port);
                RetStr=FormatStr(RetStr, "tcp:%s:%d", Address, Port);

            }
        }
    }

    Destroy(Tempstr);
    Destroy(Address);
    Destroy(Token);

    return(RetStr);
}



static TFileItem *FTP_FileInfo(TFileStore *FS, const char *Path)
{
    TFileItem *Item;

    return(Item);
}







static ListNode *FTP_ListDir(TFileStore *FS, const char *Path)
{
    int result, MLSD=FALSE;
    char *Tempstr=NULL, *URL=NULL;
    TFileItem *FI=NULL;
    ListNode *Files=NULL;
    STREAM *S=NULL;

    MLSD=FTP_HasFeature(FS,"MLSD");
    URL=FTP_NegotiatePassiveDataConnection(URL, FS);
    if (! StrValid(URL)) return(NULL);

    if (MLSD) SendLoggedLine("MLSD\r\n", FS, FS->S);
    else SendLoggedLine("LIST\r\n", FS, FS->S);

    S=FTP_MakePassiveDataConnection(FS, URL);
    if (InetReadResponse(FS->S, FS, &Tempstr, NULL, INET_CONTINUE) && S)
    {
        Files=ListCreate();
        Tempstr=ReadLoggedLine(Tempstr, FS, S);
        while (Tempstr)
        {
            StripLeadingWhitespace(Tempstr);

            if (StrValid(Tempstr))
            {
                if (MLSD) FI=Decode_MLSD_Output(FS->CurrDir,Tempstr);
                else FI=Decode_LS_Output(FS->CurrDir,Tempstr);

                if (FI)
                {
                    //we don't use absolute paths with ftp
                    FI->path=CopyStr(FI->path,FI->name);
                    ListAddNamedItem(Files, FI->name, FI);
                }
            }

            Tempstr=ReadLoggedLine(Tempstr, FS, S);
        }
        //Read 'End of transfer'
        InetReadResponse(FS->S, FS, &Tempstr, NULL, INET_OKAY);
    }

    if (S) STREAMClose(S);

    Destroy(Tempstr);

    return(Files);
}



static const char *FTP_CurrDir(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    int result=FALSE;

    Tempstr=FormatStr(Tempstr,"PWD\r\n");
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY))
    {
        result=TRUE;
        GetToken(Verbiage, "\\S", &FS->CurrDir, GETTOKEN_QUOTES);
    }

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(FS->CurrDir);
}



static int FTP_ChDir(TFileStore *FS, const char *Dir)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    int result=FALSE;

    if (strcmp(Dir,"..")==0) SendLoggedLine("CDUP\r\n", FS, FS->S);
    else
    {
        Tempstr=MCopyStr(Tempstr,"CWD ", Dir, "\r\n", NULL);
        SendLoggedLine(Tempstr, FS, FS->S);
    }

    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY)) result=TRUE;

    FTP_CurrDir(FS);

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(result);
}


static int FTP_MkDir(TFileStore *FS, const char *Dir, int Mkdir)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=FormatStr(Tempstr,"MKD %s\r\n",Dir);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,NULL, INET_OKAY)) result=TRUE;

    Destroy(Tempstr);

    return(result);
}



static int FTP_Symlink(TFileStore *FS, char *FromPath, char *ToPath)
{
    int result=FALSE;
    char *Tempstr=NULL, *Verbiage=NULL;

    Tempstr=MCopyStr(Tempstr, "SITE SYMLINK ", FromPath, " ", ToPath, "\r\n", NULL);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,&Verbiage, INET_OKAY))
    {
        //HandleEventMessage(Settings.Flags,"%s %s\n",Tempstr,Verbiage);
        result=TRUE;
    }

    Destroy(Verbiage);
    Destroy(Tempstr);

    return(result);
}


static int FTP_RmDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=FormatStr(Tempstr,"RMD %s\r\n", Path);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,NULL, INET_OKAY)) result=TRUE;

    Destroy(Tempstr);

    return(result);
}


static int FTP_Unlink(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=FormatStr(Tempstr, "DELE %s\r\n", Path);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,NULL, INET_OKAY)) result=TRUE;
    else result=FTP_RmDir(FS, Path);

    Destroy(Tempstr);

    return(result);
}



static int FTP_Rename(TFileStore *FS, const char *FromPath, const char *ToPath)
{
    int result=FALSE;
    char *Tempstr=NULL, *Verbiage=NULL, *FileName=NULL;
    char *CorrectedToPath=NULL;
    const char *ptr;
    TFileItem *FI;

    Tempstr=MCopyStr(Tempstr,"RNFR ",FromPath,"\r\n",NULL);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,NULL,INET_OKAY|INET_MORE))
    {
        ptr=strrchr(FromPath,'/');
        if (ptr) ptr++;
        else ptr=FromPath;

        FileName=CopyStr(FileName,ptr);

        //Handle '.' meaning 'move to current dir'
        if (strcmp(ToPath,".")==0)
        {
            CorrectedToPath=CopyStr(CorrectedToPath,ptr);
        }
        else if (strcmp(ToPath,"..")==0) CorrectedToPath=MCopyStr(CorrectedToPath,"../",FileName,NULL);
        else CorrectedToPath=CopyStr(CorrectedToPath,ToPath);

        Tempstr=MCopyStr(Tempstr,"RNTO ",CorrectedToPath,"\r\n",NULL);
        SendLoggedLine(Tempstr, FS, FS->S);

        if (InetReadResponse(FS->S, FS, &Tempstr,&Verbiage, INET_OKAY))
        {
            result=TRUE;
        }
    }

    Destroy(CorrectedToPath);
    Destroy(FileName);
    Destroy(Verbiage);
    Destroy(Tempstr);

    return(result);
}





static int FTP_ChMod(TFileStore *FS, const char *Path, int Mode)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=FormatStr(Tempstr,"SITE CHMOD %d %s\r\n",Mode, Path);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,NULL, INET_OKAY)) result=TRUE;

    Destroy(Tempstr);

    return(result);
}


static int FTP_ChPassword(TFileStore *FS, const char *Old, const char *New)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=FormatStr(Tempstr,"SITE PSWD %s %s\r\n", Old, New);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr,NULL, INET_OKAY)) result=TRUE;

    Destroy(Tempstr);

    return(result);
}


static char *FTP_GetDiskUsage(char *RetStr, TFileStore *FS)
{
    float total=0, avail=0, used=0;
    char *Tempstr=NULL, *Verbiage=NULL;


    if (FTP_HasFeature(FS, "AVBL") )
    {
        if (FTP_SendCommand(FS, "AVBL\r\n", &Verbiage))
        {

            GetToken(Verbiage, "\\S", &Tempstr, 0);
            avail=atof(Tempstr);
            RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f", total, avail, used);
        }
    }

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(RetStr);
}



static char *FTP_GetHash(char *RetStr, TFileStore *FS, const char *Path, const char *Type)
{
    char *Tempstr=NULL, *Hash=NULL;

    Tempstr=MCopyStr(Tempstr, "X", Type, " ", NULL);
    strupr(Tempstr);
    Tempstr=MCatStr(Tempstr,"\"", Path, "\"\r\n", NULL);
    SendLoggedLine(Tempstr, FS, FS->S);

    if (InetReadResponse(FS->S, FS, &Tempstr, &Hash, INET_OKAY)) RetStr=CopyStr(RetStr, Hash);

    Destroy(Tempstr);
    Destroy(Hash);

    return(RetStr);
}


static char *FTP_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{

    RetStr=CopyStr(RetStr, "");
    if (StrValid(ValName))
    {
        if ( (strcasecmp(ValName, "DiskQuota")==0) ) RetStr=FTP_GetDiskUsage(RetStr, FS);
        if ( (strcasecmp(ValName, "crc")==0) ) RetStr=FTP_GetHash(RetStr, FS, Path, ValName);
        if ( (strcasecmp(ValName, "md5")==0) ) RetStr=FTP_GetHash(RetStr, FS, Path, ValName);
        if ( (strcasecmp(ValName, "sha1")==0) ) RetStr=FTP_GetHash(RetStr, FS, Path, ValName);
        if ( (strcasecmp(ValName, "sha256")==0) ) RetStr=FTP_GetHash(RetStr, FS, Path, ValName);
        if ( (strcasecmp(ValName, "sha512")==0) ) RetStr=FTP_GetHash(RetStr, FS, Path, ValName);
    }

    return(RetStr);
}



static STREAM *FTP_OpenFile(TFileStore *FS, const char *Path, const char *OpenFlags, uint64_t Size)
{
    char *Tempstr=NULL, *Verbiage=NULL, *URL=NULL;
    STREAM *S=NULL;

    URL=FTP_NegotiatePassiveDataConnection(URL, FS);
    if (StrValid(URL))
    {
        GetToken(OpenFlags, " ", &Tempstr, 0);
        if (StrValid(Tempstr) && (Tempstr[0]=='w'))
        {
            STREAMSetFlushType(FS->S, FLUSH_ALWAYS, 0, 0);
            Tempstr=FormatStr(Tempstr, "STOR %s\r\n", Path);
            SendLoggedLine(Tempstr, FS, FS->S);
        }
        else
        {
            /*
                  if ((Flags & OPEN_RESUME) && (FI->ResumePoint > 0))
                  {
                    Tempstr=FormatStr(Tempstr,"REST %d\r\n",FI->ResumePoint);
                    SendLoggedLine(Tempstr,FS->S);
                    printf("%s\n",Tempstr);
                    InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY);
                  }
            */

            Tempstr=FormatStr(Tempstr, "RETR %s\r\n", Path);
            SendLoggedLine(Tempstr, FS, FS->S);
        }
    }

    S=FTP_MakePassiveDataConnection(FS, URL);
    if (S)
    {
        STREAMSetValue(S, "FTP:TransferPath", Path);
        if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_CONTINUE | INET_OKAY)) /*do nothing */;
        else
        {
            Tempstr=MCatStr(Tempstr," ",Verbiage,NULL);
            HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, Path, "");
            STREAMClose(S);
            S=NULL;
            //  WriteLog(Tempstr);
        }
    }

    Destroy(Verbiage);
    Destroy(Tempstr);
    Destroy(URL);


    return(S);
}


static int FTP_CloseFile(TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL, *Verbiage=NULL, *Path=NULL;
    int RetVal=FALSE, val;

    //do this before stream close!
    Path=CopyStr(Path, STREAMGetValue(S, "FTP:TransferPath"));
    STREAMClose(S);

    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY)) RetVal=TRUE;
    else if (StrValid(Tempstr))
    {
        val=atoi(Tempstr);
        switch (val)
        {
        case 452:
            RetVal=ERR_FULL;
            Tempstr=FormatStr(Tempstr, "FTP TRANSFER ERROR: %d Insufficient Space: %s", val, Verbiage);
            break;
        case 530:
        case 532:
            RetVal=ERR_FORBID;
            Tempstr=FormatStr(Tempstr, "FTP TRANSFER ERROR: %d Forbidden: %s", val, Verbiage);
            break;
        case 550:
            RetVal=ERR_NOEXIST;
            Tempstr=FormatStr(Tempstr, "FTP TRANSFER ERROR: %d Does not exist: %s", val, Verbiage);
            break;
        case 552:
            RetVal=ERR_TOOBIG;
            Tempstr=FormatStr(Tempstr, "FTP TRANSFER ERROR: %d File too big: %s", val, Verbiage);
            break;
        case 553:
            RetVal=ERR_BADNAME;
            Tempstr=FormatStr(Tempstr, "FTP TRANSFER ERROR: %d Bad filename: %s", val, Verbiage);
            break;
        }
    }

    LogToFile(Settings->LogFile, "CLOSE: %s %s", Tempstr, Verbiage);

//ISSUE HERE
//    if (RetVal != FALSE) HandleEvent(FS, UI_OUTPUT_ERROR, Tempstr, Path, "");
//    else HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, Path, "");

    Destroy(Verbiage);
    Destroy(Tempstr);
    Destroy(Path);

    return(RetVal);
}



static int FTP_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMReadBytes(S, Buffer, len));
}


static int FTP_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    return(STREAMWriteBytes(S, Buffer, len));
}



//discover features supported
static void FTP_ReadFeatures(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL, *Token=NULL;
    const char *ptr;

    if (FTP_SendCommand(FS, "FEAT\r\n", &Verbiage))
    {
        ptr=GetToken(Verbiage, "\n", &Token, GETTOKEN_QUOTES);
        while (ptr)
        {
            // Features in the response start with a space
            if (StrValid(Token) && (*Token==' '))
            {
                StripLeadingWhitespace(Token);
                FS->Features=MCatStr(FS->Features,"'",Token,"' ", NULL);

                if (strncmp(Token,"XMD5", 4)==0) AppendVar(FS->Vars, "HashTypes", "md5");
                else if (strncmp(Token,"XSHA1", 5)==0) AppendVar(FS->Vars, "HashTypes", "sha1");
                else if (strncmp(Token,"XSHA256", 7)==0) AppendVar(FS->Vars, "HashTypes", "sha256");
                else if (strncmp(Token,"XSHA512", 7)==0) AppendVar(FS->Vars, "HashTypes", "sha512");
                else if (strncmp(Token,"XCRC", 4)==0) AppendVar(FS->Vars, "HashTypes", "crc");
                if (strcmp(Token,"SITE CHMOD")==0) FS->ChMod=FTP_ChMod;


                /*
                      if (strcmp(Token,"MODE Z")==0) FS->Features |= FS_TRANSFER_TYPES;
                      if (strcmp(Token,"REST STREAM")==0) FS->Features |= FS_RESUME_TRANSFERS;

                      if (strcmp(Token,"SITE SYMLINK")==0) FS->Features |= FS_SYMLINKS;
                */
            }
            ptr=GetToken(ptr,"\n",&Token, GETTOKEN_QUOTES);
        }
    }


    Destroy(Verbiage);
    Destroy(Tempstr);
    Destroy(Token);
}




static int FTP_SendPassword(TFileStore *FS, const char *Pass, int PassLen, int Try, char **LoginBanner)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    int result=FALSE;

    //Don't use STREAMWriteLoggedLine for the password, as we don't want to display it
    STREAMWriteLine("PASS ", FS->S);
    STREAMWriteBytes(FS->S, Pass, PassLen);
    STREAMWriteLine("\r\n", FS->S);

    STREAMFlush(FS->S);

    InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY);
    //if the password is accepted, we'll get 230 here
    if (strcmp(Tempstr,"230")==0)
    {
        if (StrLen(Verbiage) > 4) *LoginBanner=MCatStr(*LoginBanner,"\n",Verbiage,NULL);
        result=TRUE;
    }
    else
    {
        //HandleEventMessage(Settings.Flags,"LOGIN ERROR: %s",Verbiage);
        SetVar(FS->Vars,"Error",Verbiage);
    }

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(result);
}


static int FTP_Login(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL, *LoginBanner=NULL;
    const char *p_User=NULL, *p_Pass=NULL;
    int result=FALSE, len, Try;

    LoginBanner=CopyStr(LoginBanner,GetVar(FS->Vars,"LoginBanner"));
    if (StrValid(FS->User))
    {
        p_User=FS->User;
        p_Pass=FS->Pass;
    }
    else
    {
        p_User="anonymous";
        p_Pass="anonymous";
    }

    Tempstr=FormatStr(Tempstr,"USER %s\r\n",p_User);
    SendLoggedLine(Tempstr, FS, FS->S);

    InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY | INET_CONTINUE);
    //some FTP systems allow users to enter as 'anonymous' or other password-less
    //accounts, so we might get '230 Logged in' after sending the username
    if (strcmp(Tempstr,"230")==0)
    {
        LoginBanner=CatStr(LoginBanner,Verbiage);
        result=TRUE;
    }
    else
    {
        //331 means 'send password'
        if (strcmp(Tempstr,"331")==0)
        {
            for (Try=0; Try < 5; Try++)
            {
                if (p_Pass==NULL) len=PasswordGet(FS, Try, &p_Pass);
                else len=StrLen(p_Pass);

                result=FTP_SendPassword(FS, p_Pass, len, Try, &LoginBanner);
                if (result) break;
                p_Pass=NULL;
            }
        }
    }

    SetVar(FS->Vars,"LoginBanner",LoginBanner);

    Destroy(LoginBanner);
    Destroy(Tempstr);
    Destroy(Verbiage);

    return(result);
}





static int FTP_AuthTLS(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;

    SendLoggedLine("AUTH TLS\r\n", FS, FS->S);
    if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY))
    {
        DoSSLClientNegotiation(FS->S, 0);
        FS->Flags |= FILESTORE_TLS;
        FileStoreRecordCipherDetails(FS, FS->S);
    }

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(TRUE);
}


static int FTP_ProtP(TFileStore *FS)
{
    char *Tempstr=NULL, *Verbiage=NULL;

    SendLoggedLine("PROT P\r\n", FS, FS->S);
    InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY);

    Destroy(Tempstr);
    Destroy(Verbiage);
}



static int FTP_Connect(TFileStore *FS)
{
    char *Proto=NULL, *Host=NULL, *PortStr=NULL, *Path=NULL, *Tempstr=NULL, *Verbiage=NULL;
    char *ptr;
    int RetVal=FALSE;


    ParseURL(FS->URL, &Proto, &Host, &PortStr, NULL, NULL, &Path,  NULL);
    if (! StrValid(PortStr)) PortStr=CopyStr(PortStr, "21");

    Tempstr=MCopyStr(Tempstr, "tcp:", Host, ":", PortStr, NULL);
    FS->S=STREAMOpen(Tempstr, "");
    if (FS->S)
    {
        RetVal=TRUE;

        STREAMSetTimeout(FS->S,3000);
        if (InetReadResponse(FS->S, FS, &Tempstr, &Verbiage, INET_OKAY))
        {
            if (StrLen(Verbiage) > 4) SetVar(FS->Vars,"ServerBanner",Verbiage);


            if (StrValid(Tempstr) && (*Tempstr=='2'))
            {
                if (strcasecmp(Proto, "ftps")==0) FTP_AuthTLS(FS);

                RetVal=FTP_Login(FS);
                while (! RetVal)
                {
                    FS->Pass=UI_AskPassword(FS->Pass);
                    RetVal=FTP_Login(FS);
                    HandleEvent(FS, 0, "$(filestore): Login Failed.", FS->URL, "");
                }
            }
            else
            {
                SetVar(FS->Vars,"ServerError",Verbiage);
                HandleEvent(FS, 0, "$(filestore): FTP server error.", FS->URL, "");
            }
        }
    }


    if (RetVal)
    {
        FTP_ReadFeatures(FS);
        FTP_CurrDir(FS);
        if (FS->Flags & FILESTORE_TLS) FTP_ProtP(FS);
        if (FTP_HasFeature(FS, "AVBL")) FS->Flags |= FILESTORE_USAGE;
        FileStoreGetTimeFromFile(FS);
    }
    else
    {
        HandleEvent(FS, 0, "$(filestore): Connection Failed.", FS->URL, "");
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


static int FTP_Disconnect(TFileStore *FS)
{
    char *Tempstr=NULL;
    int result;

    if (FS->S)
    {
        SendLoggedLine("QUIT\r\n", FS, FS->S);
        InetReadResponse(FS->S, FS, &Tempstr, NULL, INET_OKAY);
        STREAMClose(FS->S);
        FS->S=NULL;
    }
    Destroy(Tempstr);

    return(TRUE);
}



int FTP_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS;

    FS->Connect=FTP_Connect;
    FS->ListDir=FTP_ListDir;
    //FS->FileInfo=FTP_FileInfo;
    FS->MkDir=FTP_MkDir;
    FS->ChDir=FTP_ChDir;
    FS->UnlinkPath=FTP_Unlink;
    FS->RenamePath=FTP_Rename;
    FS->OpenFile=FTP_OpenFile;
    FS->CloseFile=FTP_CloseFile;
    FS->ReadBytes=FTP_ReadBytes;
    FS->WriteBytes=FTP_WriteBytes;
    FS->GetValue=FTP_GetValue;
}
