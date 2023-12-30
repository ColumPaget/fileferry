#include "commands.h"
#include "ui.h"
#include "settings.h"
#include "help.h"
#include "file_include_exclude.h"
#include "errors_and_logging.h"
#include "filestore.h"
#include "filestore_dirlist.h"

char *CommandFileLoad(char *RetStr, const char *Path)
{
    STREAM *S;
    char *Tempstr=NULL;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            RetStr=MCatStr(RetStr, Tempstr, "; ", NULL);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    return(RetStr);
}


//this function gets a command argument that is intended as a file-extension
//if it doesn't start with '.' then '.' is added
const char *CommandLineGetExtn(const char *CommandLine, char **Extn)
{
    char *Token=NULL;
    const char *ptr;

    ptr=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
    if (*Token != '.') *Extn=MCopyStr(*Extn, ".", Token, NULL);
    else *Extn=CopyStr(*Extn, Token);

    Destroy(Token);
    return(ptr);
}


//CommandLine here is command line after the switch
const char *ParseCommandSwitch(const char *CommandLine, TCommand *Cmd, const char *Switch)
{
    char *Token=NULL;
    const char *ptr;
    long val;

    if (strcmp(Switch, "-a")==0) Cmd->Flags |= CMD_FLAG_ALL;
    else if (strcmp(Switch, "-A")==0) Cmd->Flags |= CMD_FLAG_ABORT;
    else if (strcmp(Switch, "-Q")==0) Cmd->Flags |= CMD_FLAG_QUIT;
    else if (strcmp(Switch, "-F")==0) Cmd->Flags |= CMD_FLAG_FORCE;
    else if (strcmp(Switch, "-l")==0) Cmd->Flags |= CMD_FLAG_LONG;
    else if (strcmp(Switch, "-ll")==0) Cmd->Flags |= CMD_FLAG_LONG | CMD_FLAG_LONG_LONG;
    else if (strcmp(Switch, "-r")==0) Cmd->Flags |= CMD_FLAG_RECURSE;
    else if (strcmp(Switch, "-S")==0) Cmd->Flags |= CMD_FLAG_SORT_SIZE;
    else if (strcmp(Switch, "-page")==0) Cmd->Flags |= CMD_FLAG_PAGE;
    else if (strcmp(Switch, "-pg")==0) Cmd->Flags |= CMD_FLAG_PAGE;
    else if (strcmp(Switch, "-files")==0) Cmd->Flags |= CMD_FLAG_FILES_ONLY;
    else if (strcmp(Switch, "-dirs")==0) Cmd->Flags |= CMD_FLAG_DIRS_ONLY;
    else if (strcmp(Switch, "-i")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        Cmd->Includes=MCatStr(Cmd->Includes, "'",  Token, "' ", NULL);
    }
    else if (strcmp(Switch, "-x")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        Cmd->Excludes=MCatStr(Cmd->Excludes, "'",  Token, "' ", NULL);
    }
    else if (strcmp(Switch, "-newer")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        SetVar(Cmd->Vars, "Time:Newer", Token);
    }
    else if (strcmp(Switch, "-older")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        SetVar(Cmd->Vars, "Time:Older", Token);
    }
    else if (strcmp(Switch, "-mtime")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        val=atoi(Token);
        Token=CopyStr(Token, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", Now+val, NULL));

        if (val < 0) SetVar(Cmd->Vars, "Time:Newer", Token);
        else SetVar(Cmd->Vars, "Time:Older", Token);
    }
    else if (strcmp(Switch, "-larger")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        SetVar(Cmd->Vars, "Size:Larger", Token);
    }
    else if (strcmp(Switch, "-smaller")==0)
    {
        CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
        SetVar(Cmd->Vars, "Size:Smaller", Token);
    }


    switch (Cmd->Type)
    {
    case CMD_LS:
        if (strcmp(Switch, "-t")==0) Cmd->Flags |= CMD_FLAG_SORT_TIME;
        else if (strcmp(Switch, "-lt")==0) Cmd->Flags |= CMD_FLAG_SORT_TIME | CMD_FLAG_LONG;
        else if (strcmp(Switch, "-f")==0) Cmd->Flags |= CMD_FLAG_FILES_ONLY;
        else if (strcmp(Switch, "-d")==0) Cmd->Flags |= CMD_FLAG_DIRS_ONLY;
        else if (strcmp(Switch, "-n")==0)
        {
            CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
            Cmd->NoOfItems = atoi(Token);
        }
        break;

    case CMD_EXISTS:
        if (strcmp(Switch, "-f")==0) Cmd->Flags |= CMD_FLAG_FILES_ONLY;
        else if (strcmp(Switch, "-d")==0) Cmd->Flags |= CMD_FLAG_DIRS_ONLY;
        else if (strcmp(Switch, "-file")==0) Cmd->Flags |= CMD_FLAG_FILES_ONLY;
        else if (strcmp(Switch, "-dir")==0) Cmd->Flags |= CMD_FLAG_DIRS_ONLY;
        else if (strcmp(Switch, "-no")==0) Cmd->Flags |= CMD_FLAG_INVERT;
        break;

    case CMD_LOCK:
    case CMD_LLOCK:
        if (strcmp(Switch, "-w")==0) Cmd->Flags |= CMD_FLAG_WAIT;
        else if (strcmp(Switch, "-wait")==0) Cmd->Flags |= CMD_FLAG_WAIT;
        break;

    case CMD_SHOW:
        if (strcmp(Switch, "-img")==0) Cmd->Flags |= CMD_FLAG_IMG;
        if (Settings->Flags & SETTING_SIXEL) Cmd->Flags |= CMD_FLAG_SIXEL;
        if (strcmp(Switch, "-sixel")==0) Cmd->Flags |= CMD_FLAG_SIXEL;
        break;

    case CMD_PUT:
    case CMD_MPUT:
        if (strcmp(Switch, "-expire")==0)
        {
            CommandLine=GetToken(CommandLine, " ", &Token, GETTOKEN_QUOTES);
            SetVar(Cmd->Vars, "Expire", Token);
        }
    //fall through to GET
    case CMD_GET:
    case CMD_MGET:
        if (strcmp(Switch,"-s")==0) Cmd->Flags |= CMD_FLAG_SYNC;
        else if (strcmp(Switch, "-f")==0) Cmd->Flags |= CMD_FLAG_FORCE;
        else if (strcmp(Switch, "-n")==0)
        {
            CommandLine=GetToken(CommandLine, "\\S", &Token, GETTOKEN_QUOTES);
            Cmd->NoOfItems = atoi(Token);
        }
        else if (strcmp(Switch,"-enc")==0) Cmd->EncryptType = ENCRYPT_ANY;
        else if (strcmp(Switch,"-ssl")==0) Cmd->EncryptType = ENCRYPT_OPENSSL_PW;
        else if (strcmp(Switch,"-sslenc")==0) Cmd->EncryptType = ENCRYPT_OPENSSL_PW;
        else if (strcmp(Switch,"-gpg")==0) Cmd->EncryptType = ENCRYPT_GPG_PW;
        else if (strcmp(Switch,"-zenc")==0) Cmd->EncryptType = ENCRYPT_ZIP;
        else if (strcmp(Switch,"-7zenc")==0) Cmd->EncryptType = ENCRYPT_7ZIP;
        else if (strcmp(Switch, "-t")==0) SetVar(Cmd->Vars, "DestTransferExtn", ".tmp");
        else if (strcmp(Switch, "-h")==0)
        {
            CommandLine=GetToken(CommandLine, " ", &Token, GETTOKEN_QUOTES);
            SetVar(Cmd->Vars, "PostTransferHookScript", Token);
        }
        else if (strcmp(Switch, "-posthook")==0)
        {
            CommandLine=GetToken(CommandLine, " ", &Token, GETTOKEN_QUOTES);
            SetVar(Cmd->Vars, "PostTransferHookScript", Token);
        }
        else if (strcmp(Switch, "-prehook")==0)
        {
            CommandLine=GetToken(CommandLine, " ", &Token, GETTOKEN_QUOTES);
            SetVar(Cmd->Vars, "PreTransferHookScript", Token);
        }
        else if (strcmp(Switch, "-ed")==0)
        {
            CommandLine=CommandLineGetExtn(CommandLine, &Token);
            SetVar(Cmd->Vars, "DestFinalExtn", Token);
        }
        else if (strcmp(Switch, "-es")==0)
        {
            CommandLine=CommandLineGetExtn(CommandLine, &Token);
            SetVar(Cmd->Vars, "SourceFinalExtn", Token);
        }
        else if (strcmp(Switch, "-et")==0)
        {
            CommandLine=CommandLineGetExtn(CommandLine, &Token);
            SetVar(Cmd->Vars, "DestTransferExtn", Token);
        }
        else if (strcmp(Switch, "-Ed")==0)
        {
            CommandLine=CommandLineGetExtn(CommandLine, &Token);
            SetVar(Cmd->Vars, "DestAppendExtn", Token);
        }
        else if (strcmp(Switch, "-Es")==0)
        {
            CommandLine=CommandLineGetExtn(CommandLine, &Token);
            SetVar(Cmd->Vars, "SourceAppendExtn", Token);
        }
        else if (strcmp(Switch, "-key")==0)
        {
            CommandLine=GetToken(CommandLine, " ", &Token, GETTOKEN_QUOTES);
            Cmd->EncryptKey=CopyStr(Cmd->EncryptKey, Token);
        }
        else if (strcmp(Switch, "-bak")==0)
        {
            CommandLine=GetToken(CommandLine, " ", &Token, GETTOKEN_QUOTES);
            SetVar(Cmd->Vars, "DestBackups", Token);
        }
        break;
    }

    Destroy(Token);

    return(CommandLine);
}



int CommandMatch(const char *Str)
{
    int Cmd=CMD_NONE;

    if (strcmp(Str, "cd")==0) Cmd=CMD_CD;
    else if (strcmp(Str, "chdir")==0) Cmd=CMD_CD;
    else if (strcmp(Str, "lcd")==0) Cmd=CMD_LCD;
    else if (strcmp(Str, "lchdir")==0) Cmd=CMD_LCD;
    else if (strcmp(Str, "mkdir")==0) Cmd=CMD_MKDIR;
    else if (strcmp(Str, "lmkdir")==0) Cmd=CMD_LMKDIR;
    else if (strcmp(Str, "rmdir")==0) Cmd=CMD_RMDIR;
    else if (strcmp(Str, "lrmdir")==0) Cmd=CMD_LRMDIR;
    else if (strcmp(Str, "rm")==0)  Cmd=CMD_DEL;
    else if (strcmp(Str, "lrm")==0)  Cmd=CMD_LDEL;
    else if (strcmp(Str, "del")==0) Cmd=CMD_DEL;
    else if (strcmp(Str, "ldel")==0) Cmd=CMD_LDEL;
    else if (strcmp(Str, "ls")==0)  Cmd=CMD_LS;
    else if (strcmp(Str, "lls")==0) Cmd=CMD_LLS;
    else if (strcmp(Str, "stat")==0)  Cmd=CMD_STAT;
    else if (strcmp(Str, "lstat")==0) Cmd=CMD_LSTAT;
    else if (strcmp(Str, "stats")==0)  Cmd=CMD_STAT;
    else if (strcmp(Str, "lstats")==0) Cmd=CMD_LSTAT;
    else if (strcmp(Str, "get")==0) Cmd=CMD_GET;
    else if (strcmp(Str, "put")==0) Cmd=CMD_PUT;
    else if (strcmp(Str, "mget")==0) Cmd=CMD_MGET;
    else if (strcmp(Str, "mput")==0) Cmd=CMD_MPUT;
    else if (strcmp(Str, "show")==0) Cmd=CMD_SHOW;
    else if (strcmp(Str, "lshow")==0) Cmd=CMD_LSHOW;
    else if (strcmp(Str, "share")==0) Cmd=CMD_SHARE;
    else if (strcmp(Str, "pwd")==0) Cmd=CMD_PWD;
    else if (strcmp(Str, "lpwd")==0) Cmd=CMD_LPWD;
    else if (strcmp(Str, "cp")==0) Cmd=CMD_COPY;
    else if (strcmp(Str, "lcp")==0) Cmd=CMD_LCOPY;
    else if (strcmp(Str, "copy")==0) Cmd=CMD_COPY;
    else if (strcmp(Str, "lcopy")==0) Cmd=CMD_LCOPY;
    else if (strcmp(Str, "mv")==0) Cmd=CMD_RENAME;
    else if (strcmp(Str, "lmv")==0) Cmd=CMD_LRENAME;
    else if (strcmp(Str, "move")==0) Cmd=CMD_RENAME;
    else if (strcmp(Str, "lmove")==0) Cmd=CMD_LRENAME;
    else if (strcmp(Str, "rename")==0) Cmd=CMD_RENAME;
    else if (strcmp(Str, "chext")==0) Cmd=CMD_CHEXT;
    else if (strcmp(Str, "lchext")==0) Cmd=CMD_LCHEXT;
    else if (strcmp(Str, "chmod")==0) Cmd=CMD_CHMOD;
    else if (strcmp(Str, "crc")==0) Cmd=CMD_CRC;
    else if (strcmp(Str, "lcrc")==0) Cmd=CMD_LCRC;
    else if (strcmp(Str, "md5")==0) Cmd=CMD_MD5;
    else if (strcmp(Str, "lmd5")==0) Cmd=CMD_LMD5;
    else if (strcmp(Str, "sha1")==0) Cmd=CMD_SHA1;
    else if (strcmp(Str, "lsha1")==0) Cmd=CMD_LSHA1;
    else if (strcmp(Str, "info")==0) Cmd=CMD_INFO;
    else if (strcmp(Str, "exists")==0) Cmd=CMD_EXISTS;
    else if (strcmp(Str, "lock")==0) Cmd=CMD_LOCK;
    else if (strcmp(Str, "llock")==0) Cmd=CMD_LLOCK;
    else if (strcmp(Str, "unlock")==0) Cmd=CMD_UNLOCK;
    else if (strcmp(Str, "lunlock")==0) Cmd=CMD_LUNLOCK;
    else if (strcmp(Str, "chpassword")==0) Cmd=CMD_CHPASSWORD;
    else if (strcmp(Str, "password")==0) Cmd=CMD_CHPASSWORD;
    else if (strcmp(Str, "chpasswd")==0) Cmd=CMD_CHPASSWORD;
    else if (strcmp(Str, "passwd")==0) Cmd=CMD_CHPASSWORD;
    else if (strcmp(Str, "set")==0) Cmd=CMD_SET;
    else if (strcmp(Str, "help")==0) Cmd=CMD_HELP;
    else if (strcmp(Str, "quit")==0) Cmd=CMD_QUIT;
    else if (strcmp(Str, "exit")==0) Cmd=CMD_QUIT;

    return(Cmd);
}


TCommand *CommandParse(const char *Str)
{
    TCommand *Cmd;
    char *Token=NULL;
    const char *ptr;
    int SwitchesActive=TRUE;

    Cmd=(TCommand *) calloc(1, sizeof(TCommand));
    Cmd->Vars=ListCreate();
    ptr=GetToken(Str, "\\S", &Token, 0);
    Cmd->Type=CommandMatch(Token);

    switch (Cmd->Type)
    {
    //some commands have no switches and only take one argument
    case CMD_CD:
    case CMD_LCD:
        Cmd->Target=CopyStr(Cmd->Target, ptr);
        break;

    case CMD_SET:
        ptr=GetToken(ptr, "\\S", &Token, 0);
        if (! StrValid(Token)) UI_ShowSettings();
        else SettingChange(Token, ptr);
        break;


    default:
        ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
        while (ptr)
        {
            if ((*Token == '-') && SwitchesActive)
            {
                if (strcmp(Token, "--")==0) SwitchesActive=FALSE;
                else ptr=ParseCommandSwitch(ptr, Cmd, Token);
            }
            else
            {
                switch (Cmd->Type)
                {
                //some commands have many file arguments
                case CMD_MPUT:
                case CMD_MGET:
                    Cmd->Target=MCatStr(Cmd->Target, "'", Token, "' ", NULL);
                    break;

                //some commands have one 'change' argument (file mode or extension) and many file arguments
                case CMD_CHMOD:
                case CMD_CHEXT:
                    if (StrValid(Cmd->Mode)) Cmd->Target=MCatStr(Cmd->Target, "'", Token, "' ", NULL);
                    else Cmd->Mode=CopyStr(Cmd->Mode, Token);
                    break;

                //some commands have 'src' and 'dest'
                case CMD_COPY:
                case CMD_RENAME:
                case CMD_CHPASSWORD:
                    if (StrValid(Cmd->Target)) Cmd->Dest=CopyStr(Cmd->Dest, Token);
                    else Cmd->Target=CopyStr(Cmd->Target, Token);
                    break;


                //default is for commands to have switches and ONE argument
                default:
                    if (StrValid(Cmd->Target)) Cmd->Target=MCatStr(Cmd->Target, " ", Token, NULL);
                    else Cmd->Target=CopyStr(Cmd->Target, Token);
                    break;
                }
            }
            ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
        }
        break;
    }

    Destroy(Token);
    return(Cmd);
}


void CommandActivateTimeout()
{
    if (Settings->CommandTimeout > 0) alarm(Settings->CommandTimeout);
    else if (Settings->ProcessTimeout > 0) alarm(Settings->ProcessTimeout);
}

void CommandDeactivateTimeout()
{
    alarm(0);
}


#define CMD_ABORT -1

#define CMD_TYPE_PATH 1
#define CMD_TYPE_DEST 2
#define CMD_TYPE_ITEM 3
#define CMD_TYPE_MODE 4

int CommandGlobAndProcess(TFileStore *FS, int CmdType, TCommand *Cmd, void *Func)
{
    ListNode *DirList, *Curr;
    TFileItem *FI;
    int result=FALSE;

    DirList=FileStoreGlob(FS, Cmd->Target);
    Curr=ListGetNext(DirList);
    if (! Curr) UI_Output(UI_OUTPUT_ERROR, "No files matching '%s'", Cmd->Target);

    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FileIncluded(Cmd, FI, FS, FS))
        {
            result=TRUE;
            switch (CmdType)
            {
            case CMD_TYPE_PATH:
                result=((PATH_FUNC) Func)(FS, FI->path);
                break;
            case CMD_TYPE_ITEM:
                result=((ITEM_FUNC) Func)(FS, FI);
                break;
            case CMD_TYPE_DEST:
                result=((RENAME_FUNC) Func)(FS, FI->path, Cmd->Dest);
                break;
            case CMD_TYPE_MODE:
                //we use 'RENAME_FUNC' because that accepts two strings
                result=((RENAME_FUNC) Func)(FS, Cmd->Mode, FI->path);
                break;
            }
            if ((! result) && (Cmd->Flags & CMD_FLAG_ABORT)) return(CMD_ABORT);
        }
        Curr=ListGetNext(Curr);
    }

    FileStoreDirListFree(FS, DirList);

    return(result);
}



int CommandProcess(TCommand *Cmd, TFileStore *LocalFS, TFileStore *RemoteFS)
{
    char *Tempstr=NULL;
    int result=TRUE;

    CommandActivateTimeout();
    switch (Cmd->Type)
    {
    case CMD_HELP:
        HelpCommand(Cmd->Target);
        break;

    case CMD_INFO:
        if (strncmp(Cmd->Target, "encrypt", 7)==0) FileStoreOutputCipherDetails(RemoteFS, 0);
        if (strcmp(Cmd->Target, "usage")==0) FileStoreOutputDiskQuota(RemoteFS);
        if (strcmp(Cmd->Target, "features")==0) FileStoreOutputSupportedFeatures(RemoteFS);
        //if (strcmp(Cmd->Target, "service")==0) FileStoreOutputServiceInfo(RemoteFS);
        break;

    case CMD_EXISTS:
        result=FileStoreItemExists(RemoteFS, Cmd->Target, Cmd->Flags);
        if (result) UI_Output(UI_OUTPUT_ERROR, "'%s' exists", Cmd->Target);
        else UI_Output(UI_OUTPUT_ERROR, "'%s' does not exist", Cmd->Target);
        break;

    case CMD_LEXISTS:
        result=FileStoreGlobCount(LocalFS, Cmd->Target);
        if (result) UI_Output(UI_OUTPUT_ERROR, "'%s' exists", Cmd->Target);
        else UI_Output(UI_OUTPUT_ERROR, "'%s' does not exist", Cmd->Target);
        break;

    case CMD_CD:
        result=FileStoreChDir(RemoteFS, Cmd->Target);
        break;

    case CMD_LCD:
        result=FileStoreChDir(LocalFS, Cmd->Target);
        break;

    case CMD_DEL:
        result=CommandGlobAndProcess(RemoteFS, CMD_TYPE_ITEM, Cmd, FileStoreUnlinkItem);
        break;

    case CMD_LDEL:
        result=CommandGlobAndProcess(LocalFS, CMD_TYPE_ITEM, Cmd, FileStoreUnlinkItem);
        break;

    case CMD_MKDIR:
        result=FileStoreMkDir(RemoteFS, Cmd->Target, 0700);
        break;

    case CMD_LMKDIR:
        result=FileStoreMkDir(LocalFS, Cmd->Target, 0700);
        break;

    case CMD_RMDIR:
        result=FileStoreRmDir(RemoteFS, Cmd->Target);
        break;

    case CMD_LRMDIR:
        result=FileStoreRmDir(LocalFS, Cmd->Target);
        break;


    case CMD_RENAME:
        result=CommandGlobAndProcess(RemoteFS, CMD_TYPE_DEST, Cmd, FileStoreRename);
        break;

    case CMD_LRENAME:
        result=CommandGlobAndProcess(LocalFS, CMD_TYPE_DEST, Cmd, FileStoreRename);
        break;

    case CMD_COPY:
        result=CommandGlobAndProcess(RemoteFS, CMD_TYPE_DEST, Cmd, FileStoreCopyFile);
        break;

    case CMD_LCOPY:
        result=CommandGlobAndProcess(LocalFS, CMD_TYPE_DEST, Cmd, FileStoreCopyFile);
        break;

    case CMD_LS:
        UI_OutputDirList(RemoteFS, Cmd);
        break;

    case CMD_LLS:
        UI_OutputDirList(LocalFS, Cmd);
        break;

    case CMD_STAT:
        UI_OutputFStat(RemoteFS, Cmd);
        break;

    case CMD_LSTAT:
        UI_OutputFStat(LocalFS, Cmd);
        break;

    case CMD_GET:
    case CMD_MGET:
        HandleEvent(RemoteFS, UI_OUTPUT_DEBUG, "$(filestore) GET $(path)", Cmd->Target, "");
        TransferFileCommand(RemoteFS, LocalFS, Cmd);
        break;

    case CMD_PUT:
    case CMD_MPUT:
        HandleEvent(RemoteFS, UI_OUTPUT_DEBUG, "$(filestore) PUT $(path)", Cmd->Target, "");
        TransferFileCommand(LocalFS, RemoteFS, Cmd);
        break;

    case CMD_SHOW:
        UI_ShowFile(RemoteFS, LocalFS, Cmd);
        break;

    case CMD_LSHOW:
        UI_ShowFile(LocalFS, NULL, Cmd);
        break;

    case CMD_SHARE:
        Tempstr=FileStoreGetValue(Tempstr, RemoteFS, Cmd->Target, "ShareLink");
        printf("share_url: %s\n", Tempstr);
        break;

    case CMD_PWD:
        printf("%s\n", RemoteFS->CurrDir);
        break;

    case CMD_LPWD:
        printf("%s\n", LocalFS->CurrDir);
        break;

    case CMD_CHMOD:
        result=CommandGlobAndProcess(LocalFS, CMD_TYPE_MODE, Cmd, FileStoreChMod);
        break;

    case CMD_CHPASSWORD:
        if (StrValid(Cmd->Dest)) FileStoreChPassword(RemoteFS, Cmd->Target, Cmd->Dest);
        else FileStoreChPassword(RemoteFS, "", Cmd->Target);
        break;

    case CMD_LOCK:
        result=FileStoreLock(RemoteFS, Cmd->Target, Cmd->Flags);
        break;

    case CMD_LLOCK:
        result=FileStoreLock(LocalFS, Cmd->Target, Cmd->Flags);
        break;

    case CMD_UNLOCK:
        result=FileStoreUnLock(RemoteFS, Cmd->Target);
        break;

    case CMD_LUNLOCK:
        result=FileStoreUnLock(LocalFS, Cmd->Target);
        break;

    case CMD_CRC:
        if (RemoteFS->GetValue)
        {
            Tempstr=RemoteFS->GetValue(Tempstr, RemoteFS, Cmd->Target, "crc");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_MD5:
        if (RemoteFS->GetValue)
        {
            Tempstr=RemoteFS->GetValue(Tempstr, RemoteFS, Cmd->Target, "md5");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_SHA1:
        if (RemoteFS->GetValue)
        {
            Tempstr=RemoteFS->GetValue(Tempstr, RemoteFS, Cmd->Target, "sha1");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_SHA256:
        if (RemoteFS->GetValue)
        {
            Tempstr=RemoteFS->GetValue(Tempstr, RemoteFS, Cmd->Target, "sha256");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_LCRC:
        if (LocalFS->GetValue)
        {
            Tempstr=LocalFS->GetValue(Tempstr, LocalFS, Cmd->Target, "crc");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_LMD5:
        if (LocalFS->GetValue)
        {
            Tempstr=LocalFS->GetValue(Tempstr, LocalFS, Cmd->Target, "md5");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_LSHA1:
        if (LocalFS->GetValue)
        {
            Tempstr=LocalFS->GetValue(Tempstr, LocalFS, Cmd->Target, "sha1");
            printf("%s\n", Tempstr);
        }
        break;

    case CMD_LSHA256:
        if (LocalFS->GetValue)
        {
            Tempstr=LocalFS->GetValue(Tempstr, LocalFS, Cmd->Target, "sha256");
            printf("%s\n", Tempstr);
        }
        break;
    }

    if (Cmd->Flags & CMD_FLAG_INVERT) result=! result;

    if ((! result) && (Cmd->Flags & CMD_FLAG_ABORT)) result=CMD_ABORT;
    if ((! result) && (Cmd->Flags & CMD_FLAG_QUIT)) result=CMD_QUIT;
    CommandDeactivateTimeout();

    Destroy(Tempstr);

    return(result);
}




void CommandListProcess(const char *Commands, TFileStore *LocalFS, TFileStore *RemoteFS)
{
    char *Tempstr=NULL, *Token=NULL;
    TCommand *Cmd;
    const char *ptr;
    int result;

    if (Settings->Flags & SETTING_VERBOSE) printf("PROCESS COMMANDS: [%s]\n", Commands);

    ptr=GetToken(Commands, ";", &Token, 0);
    while (ptr)
    {
        StripTrailingWhitespace(Token);
        StripLeadingWhitespace(Token);
        Cmd=CommandParse(Token);
        result=CommandProcess(Cmd, LocalFS, RemoteFS);

        if (result == CMD_QUIT) break;
        else if (result == CMD_ABORT)
        {
            HandleEvent(RemoteFS, UI_OUTPUT_ERROR, "ABORT RAISED", "", "");
            break;
        }


        CommandDestroy(Cmd);
        if (Settings->ProcessTimeout > 0)
        {
            if ((GetTime(0) - ProcessStartTime) > Settings->ProcessTimeout) exit(1);
        }
        ptr=GetToken(ptr, ";", &Token, 0);
    }

    Destroy(Tempstr);
    Destroy(Token);
}


void CommandDestroy(void *p_Cmd)
{
    TCommand *Cmd;

    Cmd=(TCommand *) p_Cmd;
    Destroy(Cmd->Target);
    Destroy(Cmd->Dest);
    //ListDestroy(Cmd->Vars, Destroy);
    free(Cmd);
}
