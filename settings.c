#include "settings.h"
#include "filestore.h"
#include "saved_filestores.h"
#include "filestore_drivers/filestore_drivers.h"
#include "commands.h"
#include "help.h"
#include "ui.h"

TSettings *Settings=NULL;

#define ACT_FILEFERRY 0
#define ACT_LIST_FILESTORES 1
#define ACT_ADD_FILESTORE 2
#define ACT_LIST_DRIVERS  3
#define ACT_LIST_COMMANDS 4
#define ACT_SAVE_CONFIG   5

void PrintVersion()
{
    printf("fileferry %s\n", PACKAGE_VERSION);
    UI_Exit(0);
}


int SettingChangeBoolean(const char *Value, unsigned int Flag)
{
    if (! StrValid(Value)) Settings->Flags ^= Flag;
    else if (strtobool(Value)) Settings->Flags |= Flag;
    else Settings->Flags &= ~Flag;

    return(Settings->Flags & Flag);
}


int SettingChange(const char *Name, const char *Value)
{
    if (strcasecmp(Name, "proxy")==0) Settings->ProxyChain=CopyStr(Settings->ProxyChain, Value);
    else if (strcasecmp(Name, "log-file")==0) Settings->LogFile=CopyStr(Settings->LogFile, Value);
    else if (strcasecmp(Name, "smtp-server")==0) Settings->SmtpServer=CopyStr(Settings->SmtpServer, Value);
    else if (strcasecmp(Name, "errors-email")==0) Settings->EmailForErrors=CopyStr(Settings->EmailForErrors, Value);
    else if (strcasecmp(Name, "timeout")==0) Settings->CommandTimeout=atoi(Value);
    else if (strcasecmp(Name, "image-size")==0) Settings->ImagePreviewSize=CopyStr(Settings->ImagePreviewSize, Value);
    else if (strcasecmp(Name, "viewers")==0) Settings->ImageViewers=CopyStr(Settings->ImageViewers, Value);
    else if (strcasecmp(Name, "sixelers")==0) Settings->Sixelers=CopyStr(Settings->Sixelers, Value);
    else if (strcasecmp(Name, "sixel")==0) SettingChangeBoolean(Value, SETTING_SIXEL);
    else if (strcasecmp(Name, "verbose")==0) SettingChangeBoolean(Value, SETTING_VERBOSE);
    else if (strcasecmp(Name, "debug")==0) SettingChangeBoolean(Value, SETTING_DEBUG);
    else if (strcasecmp(Name, "syslog")==0) SettingChangeBoolean(Value, SETTING_SYSLOG);
    else if (strcasecmp(Name, "nols")==0) SettingChangeBoolean(Value, SETTING_NO_DIR_LIST);
    else if (strcasecmp(Name, "integrity")==0) SettingChangeBoolean(Value, SETTING_INTEGRITY_CHECK);
    else if (strcasecmp(Name, "xterm-title")==0) SettingChangeBoolean(Value, SETTING_XTERM_TITLE);
    else if (strcasecmp(Name, "list-type")==0)
    {
        Settings->Flags &= ~ (SETTING_LIST_LONG | SETTING_LIST_LONG_LONG);
        if (strcasecmp(Value, "long")==0) Settings->Flags |= SETTING_LIST_LONG;
        if (strcasecmp(Value, "long long")==0) Settings->Flags |= SETTING_LIST_LONG_LONG;
        if (strcasecmp(Value, "full")==0) Settings->Flags |= SETTING_LIST_LONG_LONG;
    }
}



int SettingsLoadConfigFile(const char *PathList)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL, *Path=NULL;
    const char *ptr, *p_Paths;
    int RetVal=FALSE;

    p_Paths=GetToken(PathList, ":", &Path, 0);
    while (p_Paths)
    {
        S=STREAMOpen(Path, "r");
        if (S)
        {
            Tempstr=STREAMReadLine(Tempstr, S);
            while (Tempstr)
            {
                StripTrailingWhitespace(Tempstr);
                ptr=GetToken(Tempstr, "\\S", &Token, 0);
                SettingChange(Token, ptr);
                Tempstr=STREAMReadLine(Tempstr, S);
            }
            STREAMClose(S);
        }
        p_Paths=GetToken(p_Paths, ":", &Path, 0);
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Path);

    return(RetVal);
}


void SettingWriteToConfigFile(STREAM *S, const char *Name, const char *Value)
{
    char *Tempstr=NULL;

    if (StrValid(Name) && StrValid(Value))
    {
        Tempstr=MCopyStr(Tempstr, Name, " ", Value, "\n", NULL);
        STREAMWriteLine(Tempstr, S);
    }

    Destroy(Tempstr);
}

void BooleanSettingWriteToConfigFile(STREAM *S, const char *Name, int Value)
{
    char *Tempstr=NULL;

    if (StrValid(Name))
    {
        if (Value) Tempstr=MCopyStr(Tempstr, Name, " y\n", NULL);
        else Tempstr=MCopyStr(Tempstr, Name, " n\n", NULL);
        STREAMWriteLine(Tempstr, S);
    }

    Destroy(Tempstr);
}

void IntegerSettingWriteToConfigFile(STREAM *S, const char *Name, int Value)
{
    char *Tempstr=NULL;

    if (StrValid(Name))
    {
        Tempstr=FormatStr(Tempstr, "%s %d\n", Name, Value);
        STREAMWriteLine(Tempstr, S);
    }

    Destroy(Tempstr);
}


int SettingsSaveConfigFile(const char *Path, TSettings *Settings)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    int RetVal=FALSE;

    MakeDirPath(Path, 0770);
    S=STREAMOpen(Path, "w");
    if (S)
    {
        Tempstr=MCopyStr(Tempstr, "proxy ", Settings->ProxyChain, "\n", NULL);
        STREAMWriteLine(Tempstr, S);

        SettingWriteToConfigFile(S, "log-file", Settings->LogFile);
        SettingWriteToConfigFile(S, "proxy", Settings->ProxyChain);
        SettingWriteToConfigFile(S, "smtp-server", Settings->SmtpServer);
        SettingWriteToConfigFile(S, "errors-email", Settings->EmailForErrors);
        SettingWriteToConfigFile(S, "image-size", Settings->ImagePreviewSize);
        SettingWriteToConfigFile(S, "viewers", Settings->ImageViewers);
        SettingWriteToConfigFile(S, "sixelers", Settings->Sixelers);
        BooleanSettingWriteToConfigFile(S, "verbose", Settings->Flags & SETTING_VERBOSE);
        BooleanSettingWriteToConfigFile(S, "debug", Settings->Flags & SETTING_DEBUG);
        BooleanSettingWriteToConfigFile(S, "syslog", Settings->Flags & SETTING_SYSLOG);
        BooleanSettingWriteToConfigFile(S, "nols", Settings->Flags & SETTING_NO_DIR_LIST);
        BooleanSettingWriteToConfigFile(S, "integrity", Settings->Flags & SETTING_INTEGRITY_CHECK);
        BooleanSettingWriteToConfigFile(S, "xterm-title", Settings->Flags & SETTING_XTERM_TITLE);
        BooleanSettingWriteToConfigFile(S, "sixel", Settings->Flags & SETTING_SIXEL);
        IntegerSettingWriteToConfigFile(S, "timeout", Settings->CommandTimeout);

        if (Settings->Flags & SETTING_LIST_LONG_LONG) SettingWriteToConfigFile(S, "list-type", "full");
        else if (Settings->Flags & SETTING_LIST_LONG) SettingWriteToConfigFile(S, "list-type", "long");

        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(RetVal);
}


int SettingsSaveConfig(TSettings *Settings)
{
    char *Token=NULL;
    const char *ptr;

    ptr=GetToken(Settings->ConfigFile, ":", &Token, 0);
    while (ptr)
    {
        if (SettingsSaveConfigFile(Token, Settings)) break;
        ptr=GetToken(ptr, ":", &Token, 0);
    }

    Destroy(Token);
}



void ParseCommandLineChangeConfig(int argc, const char *argv[])
{
    CMDLINE *Cmd;
    const char *arg;
    char *Token=NULL;
    const char *ptr;
    int Changed=FALSE;

    Cmd=CommandLineParserCreate(argc, (char **) argv);
    arg=CommandLineNext(Cmd);
    while (arg)
    {
        if (strchr(arg, '='))
        {
            ptr=GetToken(arg, "=", &Token, GETTOKEN_QUOTES);
            SettingChange(Token, ptr);
            Changed=TRUE;
        }
        arg=CommandLineNext(Cmd);
    }

    if (Changed) SettingsSaveConfig(Settings);
    UI_ShowSettings();

    Destroy(Token);

    free(Cmd);
}



void ParseCommandLineAddFilestore(int argc, const char *argv[])
{
    char *Tempstr=NULL;
    TFileStore *FS;
    int i;

    if (argc > 3)
    {
//arg 2 will be filestore name, arg 3 will be url
        Tempstr=MCopyStr(Tempstr, "'", argv[2], "' ", argv[3], " ", NULL);
        for (i=4; i < argc; i++)
        {
            if (strcmp(argv[i], "-user")==0) Tempstr=MCatStr(Tempstr, "user='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-password")==0) Tempstr=MCatStr(Tempstr, "pass='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-passwd")==0) Tempstr=MCatStr(Tempstr, "pass='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-pass")==0) Tempstr=MCatStr(Tempstr, "pass='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-pw")==0) Tempstr=MCatStr(Tempstr, "pass='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-e")==0) Tempstr=MCatStr(Tempstr, "encrypt='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-encrypt")==0) Tempstr=MCatStr(Tempstr, "encrypt='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-i")==0) Tempstr=MCatStr(Tempstr, "credsfile='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-proxy")==0) Tempstr=MCatStr(Tempstr, "proxy='", argv[++i], "' ", NULL);
            else if (strcmp(argv[i], "-fsfile")==0) Settings->FileStoresPath=CopyStr(Settings->FileStoresPath, argv[++i]);
        }

        FS=FileStoreParse(Tempstr);
        SavedFileStoresAdd(FS);
    }

    Destroy(Tempstr);
}



void ParseCommandLineListDrivers(int argc, const char *argv[])
{
    TFileStoreDriver *Driver;
    ListNode *Curr;
    char *Tempstr=NULL;

    Curr=ListGetNext(FileStoreDriversList());
    while (Curr)
    {
        Driver=(TFileStoreDriver *) Curr->Item;
        Tempstr=MCopyStr(Tempstr, Curr->Tag, ":", NULL);
        printf("%-10s %-20s %s\n", Tempstr, Driver->Name, Driver->Description);
        Curr=ListGetNext(Curr);
    }

    Destroy(Tempstr);
    UI_Exit(1);
}



void ParseCommandLineDefault(CMDLINE *Cmd)
{
    const char *arg;

    //we must start with CommandLineCurr, because ParseCommandLineDefault is called only
    //when we didn't recognize the first argument on the command-line as '-config', '-filestores' etc

    arg=CommandLineCurr(Cmd);
    while (arg)
    {
        if (strcmp(arg, "-user")==0) Settings->User=CopyStr(Settings->User, CommandLineNext(Cmd));
        else if (strcmp(arg, "-password")==0) Settings->Pass=CopyStr(Settings->Pass, CommandLineNext(Cmd));
        else if (strcmp(arg, "-passwd")==0) Settings->Pass=CopyStr(Settings->Pass, CommandLineNext(Cmd));
        else if (strcmp(arg, "-pass")==0) Settings->Pass=CopyStr(Settings->Pass, CommandLineNext(Cmd));
        else if (strcmp(arg, "-pw")==0) Settings->Pass=CopyStr(Settings->Pass, CommandLineNext(Cmd));
        else if (strcmp(arg, "-c")==0) Settings->Commands=CopyStr(Settings->Commands, CommandLineNext(Cmd));
        else if (strcmp(arg, "-b")==0) Settings->Commands=CommandFileLoad(Settings->Commands, CommandLineNext(Cmd));
        else if (strcmp(arg, "-e")==0) Settings->Encryption=CopyStr(Settings->Encryption, CommandLineNext(Cmd));
        else if (strcmp(arg, "-i")==0) Settings->IdentityFile=CopyStr(Settings->IdentityFile, CommandLineNext(Cmd));
        else if (strcmp(arg, "-f")==0) Settings->ConfigFile=CopyStr(Settings->ConfigFile, CommandLineNext(Cmd));
        else if (strcmp(arg, "-conf")==0) Settings->ConfigFile=CopyStr(Settings->ConfigFile, CommandLineNext(Cmd));
        else if (strcmp(arg, "-fsfile")==0) Settings->FileStoresPath=CopyStr(Settings->FileStoresPath, CommandLineNext(Cmd));
        else if (strcmp(arg, "-l")==0) Settings->LogFile=CopyStr(Settings->LogFile, CommandLineNext(Cmd));
        else if (strcmp(arg, "-log")==0) Settings->LogFile=CopyStr(Settings->LogFile, CommandLineNext(Cmd));
        else if (strcmp(arg, "-proxy")==0) Settings->ProxyChain=CopyStr(Settings->ProxyChain, CommandLineNext(Cmd));
        else if (strcmp(arg, "-life")==0) Settings->ProcessTimeout=atoi(CommandLineNext(Cmd));
        else if (strcmp(arg, "-timeout")==0) Settings->CommandTimeout=atoi(CommandLineNext(Cmd));
        else if (strcmp(arg, "-smtp-server")==0) Settings->SmtpServer=CopyStr(Settings->SmtpServer, CommandLineNext(Cmd));
        else if (strcmp(arg, "-errors-email")==0) Settings->EmailForErrors=CopyStr(Settings->EmailForErrors, CommandLineNext(Cmd));
        else if (strcmp(arg, "-nols")==0) Settings->Flags |= SETTING_NO_DIR_LIST;
        else if (strcmp(arg, "-N")==0) Settings->Flags |= SETTING_BATCH;
        else if (strcmp(arg, "-sixel")==0) Settings->Flags |= SETTING_SIXEL;
        else if (strcmp(arg, "-integrity")==0) Settings->Flags |= SETTING_INTEGRITY_CHECK;
        else if (strcmp(arg, "-I")==0) Settings->Flags |= SETTING_INTEGRITY_CHECK;
        else if ( (strcmp(arg, "-D")==0) || (strcmp(arg, "-debug")==0) )
        {
            Settings->Flags |= SETTING_DEBUG;
            LibUsefulSetValue("HTTP:Debug", "Y");
            LibUsefulSetValue("LibUseful:Debug", "Y");
        }
        else if (strcmp(arg, "-v")==0) Settings->Flags |= SETTING_VERBOSE;
        else if (strcmp(arg, "-verb")==0) Settings->Flags |= SETTING_VERBOSE;
        else if (strcmp(arg, "-verbose")==0) Settings->Flags |= SETTING_VERBOSE;
        else if (strcmp(arg, "-version")==0) PrintVersion();
        else if (strcmp(arg, "--version")==0) PrintVersion();
        else if (strcmp(arg, "-?")==0) HelpCommandLine();
        else if (strcmp(arg, "-h")==0) HelpCommandLine();
        else if (strcmp(arg, "-help")==0) HelpCommandLine();
        else if (strcmp(arg, "--help")==0) HelpCommandLine();
        else Settings->URL=CopyStr(Settings->URL, arg);

        arg=CommandLineNext(Cmd);
    }

}

int ParseCommandLine(int argc, const char *argv[])
{
    int Act=ACT_FILEFERRY;
    CMDLINE *Cmd;
    const char *arg;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR: No command-line arguments\n");
				UI_Exit(0);
    }

    Cmd=CommandLineParserCreate(argc, (char **) argv);
    arg=CommandLineNext(Cmd);

    //must do this to prevent loading commands twice
    Settings->Commands=CopyStr(Settings->Commands, "");

    if (strcmp(arg, "-filestores")==0) Act=ACT_LIST_FILESTORES;
    else if (strcmp(arg, "-drivers")==0) Act=ACT_LIST_DRIVERS;
    else if (strcmp(arg, "-commands")==0) Act=ACT_LIST_COMMANDS;
    else if (strcmp(arg, "-add")==0) Act=ACT_ADD_FILESTORE;
    else if (strcmp(arg, "-config")==0) Act=ACT_SAVE_CONFIG;
    else  ParseCommandLineDefault(Cmd);

    free(Cmd);

    return(Act);
}





int SettingsInit(int argc, const char *argv[])
{
    int RetVal=FALSE, Act;

    Settings=(TSettings *) calloc(1, sizeof(TSettings));
    Settings->SystemConfig=CopyStr(Settings->SystemConfig, "/etc/fileferry.conf");
    Settings->ConfigFile=MCopyStr(Settings->ConfigFile, GetCurrUserHomeDir(), "/.config/fileferry/fileferry.conf", NULL);

    if (isatty(1)) Settings->Flags |= SETTING_PROGRESS;
    Settings->Flags |= SETTING_TIMESTAMPS;
    Settings->ProxyChain=CopyStr(Settings->ProxyChain, "");
    Settings->LogFile=CopyStr(Settings->LogFile, "");
    Settings->EmailForErrors=CopyStr(Settings->EmailForErrors, "");
    Settings->EmailSender=CopyStr(Settings->EmailSender, "");
    Settings->SmtpServer=CopyStr(Settings->SmtpServer, "127.0.0.1:25");
    Settings->FileStoresPath=MCopyStr(Settings->FileStoresPath, GetCurrUserHomeDir(), "/.config/fileferry/filestores.conf", ":/etc/fileferry/filestores.conf", NULL);
    Settings->ImagePreviewSize=CopyStr(Settings->ImagePreviewSize, "200x200");

    Settings->ImageViewers=CopyStr(Settings->ImageViewers, "miv2 $(path),display $(path),fim $(path),feh $(path),miv $(path),mage $(path),xv $(path),imlib2_view $(path),gqview $(path),qimageviewer $(path),links -g $(path)");
    Settings->Sixelers=CopyStr(Settings->Sixelers, "img2sixel -w $(width) $(path),convert -resize $(width)x$(height) $(path) sixel:-");

    ParseCommandLine(argc, argv);
    SettingsLoadConfigFile(Settings->SystemConfig);
    SettingsLoadConfigFile(Settings->ConfigFile);
    Act=ParseCommandLine(argc, argv);

    switch (Act)
    {
    case ACT_SAVE_CONFIG:
        ParseCommandLineChangeConfig(argc, argv);
        break;

    case ACT_LIST_FILESTORES:
        SavedFileStoresList();
        break;

    case ACT_LIST_DRIVERS:
        ParseCommandLineListDrivers(argc, argv);
        break;

    case ACT_ADD_FILESTORE:
        ParseCommandLineAddFilestore(argc, argv);
        break;

    case ACT_LIST_COMMANDS:
        HelpCommandList();
        break;

    case ACT_FILEFERRY:
        RetVal=TRUE;
        break;
    }


    return(RetVal);
}
