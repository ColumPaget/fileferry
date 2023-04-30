#include "encrypt.h"
#include "ui.h"

typedef struct
{
    char *EncryptCommand;
    char *DecryptCommand;
    char *PassPrompt;
    char *EncryptedSuffix;
} TEncryptConfig;



static int EncryptFindProgram(const char *Candidates)
{
    char *Token=NULL, *Path=NULL;
    const char *ptr;
    int RetVal=ENCRYPT_NONE;

    ptr=GetToken(Candidates, ",", &Token, 0);
    while (ptr)
    {
        Path=FindFileInPath(Path, Token, getenv("PATH"));
        if (StrValid(Path))
        {
            if (strcmp(Token, "openssl")==0) RetVal=ENCRYPT_OPENSSL_PW;
            else if (strcmp(Token, "gpg")==0) RetVal=ENCRYPT_GPG_PW;
            else if (strcmp(Token, "7zip")==0) RetVal=ENCRYPT_7ZIP;
            else if (strcmp(Token, "zip")==0) RetVal=ENCRYPT_ZIP;
            break;
        }
        ptr=GetToken(Candidates, ",", &Token, 0);
    }

    Destroy(Token);
    Destroy(Path);
    return (RetVal);
}


static TEncryptConfig *EncryptGetConfig(int EncryptionType)
{
    TEncryptConfig *Config=NULL;

    if (EncryptionType==ENCRYPT_ANY) EncryptionType=EncryptFindProgram("openssl,gpg,7zip,zip");
    if (EncryptionType==ENCRYPT_NONE) return(NULL);

    Config=(TEncryptConfig *) calloc(1, sizeof(TEncryptConfig));
    switch (EncryptionType)
    {
    case ENCRYPT_GPG_PW:
        Config->EncryptCommand=CopyStr(Config->EncryptCommand, "cmd:gpg -c --batch --passphrase-fd 0 -o $(out_path) $(in_path)");
        Config->DecryptCommand=CopyStr(Config->DecryptCommand, "cmd:gpg -d --batch --passphrase-fd 0 -o $(out_path) $(in_path)");
        Config->EncryptedSuffix=CopyStr(Config->EncryptedSuffix, ".gpg");
        Config->PassPrompt=CopyStr(Config->PassPrompt, "");
        break;

    case ENCRYPT_OPENSSL_PW:
        Config->EncryptCommand=CopyStr(Config->EncryptCommand, "cmd:openssl enc -aes-256-cbc -md sha256 -pbkdf2 -in $(in_path) -out $(out_path)");
        Config->DecryptCommand=CopyStr(Config->DecryptCommand, "cmd:openssl enc -d -aes-256-cbc -md sha256 -pbkdf2 -in $(in_path) -out $(out_path)");
        Config->EncryptedSuffix=CopyStr(Config->EncryptedSuffix, "");
        Config->PassPrompt=CopyStr(Config->PassPrompt, " password:");
        break;

    case ENCRYPT_ZIP:
        Config->EncryptCommand=CopyStr(Config->EncryptCommand, "cmd:zip -e $(out_path) $(in_path)");
        Config->DecryptCommand=CopyStr(Config->DecryptCommand, "cmd:unzip $(in_path)");
        Config->EncryptedSuffix=CopyStr(Config->EncryptedSuffix, "");
        Config->PassPrompt=CopyStr(Config->PassPrompt, " password:");
        break;

    case ENCRYPT_7ZIP:
        Config->EncryptCommand=CopyStr(Config->EncryptCommand, "cmd:7za a -p $(out_path) $(in_path)");
        Config->DecryptCommand=CopyStr(Config->DecryptCommand, "cmd:7za x $(in_path)");
        Config->EncryptedSuffix=CopyStr(Config->EncryptedSuffix, "");
        Config->PassPrompt=CopyStr(Config->PassPrompt, " password");
        break;

    }

    return(Config);
}


static TEncryptConfig *DecryptGetConfig(const char *FilePath, int Type)
{
    STREAM *S;
    char *Tempstr=NULL;
    int len;

    if (Type==ENCRYPT_ANY)
    {
        S=STREAMOpen(FilePath, "r");
        Tempstr=SetStrLen(Tempstr, 10);
        len=STREAMReadBytes(S, Tempstr, 8);
        if (len == 8)
        {
            if (memcmp(Tempstr, "PK\003\004", 4)==0) Type=ENCRYPT_ZIP;
            else if (memcmp(Tempstr, "7z\xBC\xAF", 4)==0)  Type=ENCRYPT_7ZIP;
            else if (memcmp(Tempstr, "Salted__", 8)==0)  Type=ENCRYPT_OPENSSL_PW;
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(EncryptGetConfig(Type));
}


static void EncryptConfigDestroy(TEncryptConfig *Config)
{
    if (Config)
    {
        Destroy(Config->EncryptCommand);
        Destroy(Config->DecryptCommand);
        Destroy(Config->PassPrompt);
        Destroy(Config->EncryptedSuffix);
        Destroy(Config);
    }
}




static char *EncryptFileGetPath(char *RetStr, const char *SrcPath)
{
    RetStr=MCopyStr(RetStr, GetCurrUserHomeDir(), "/.tmp/", GetBasename(SrcPath), ".enc", NULL);

    return(RetStr);
}


static void SendPassPhrase(const char *PassPhrase, STREAM *S)
{
    STREAMWriteLine(PassPhrase, S);
    STREAMWriteLine("\n", S);
    STREAMFlush(S);
}

void RunExternalCommand(const char *Command, const char *PassPrompt, const char *PassPhrase, const char *Path, const char *OutPath)
{
    char *Cmd=NULL, *Args=NULL, *Tempstr=NULL;
    ListNode *Vars=NULL;
    STREAM *S;

    Vars=ListCreate();
    SetVar(Vars, "in_path", Path);
    SetVar(Vars, "out_path", OutPath);
    MakeDirPath(OutPath, 0700);

    Cmd=SubstituteVarsInString(Cmd, Command, Vars, 0);

    Tempstr=CopyStr(Tempstr, OutPath);
    StrRTruncChar(Tempstr, '/');
    Args=MCopyStr(Args, "rw pty noshell dir=", Tempstr, NULL);

    printf("Run External Command: %s\n", Cmd);
    S=STREAMOpen(Cmd, Args);
    if (S)
    {
        if (! StrValid(PassPrompt)) SendPassPhrase(PassPhrase, S);

        //the only thing we should ever see coming in on 'S' is the passphrase prompt,
        //and maybe a load of debugging/status text
        Tempstr=STREAMReadToTerminator(Tempstr, S, ':');
        while (Tempstr)
        {
            printf("PP: [%s] [%s]\n", PassPrompt, Tempstr);
            if (StrValid(PassPrompt) && strstr(Tempstr, PassPrompt)) SendPassPhrase(PassPhrase, S);
            Tempstr=STREAMReadToTerminator(Tempstr, S, ':');
        }
        STREAMClose(S);
    }

    ListDestroy(Vars, Destroy);
    Destroy(Tempstr);
    Destroy(Cmd);
    Destroy(Args);
}



char *EncryptFile(char *RetStr, const char *Path, const char *PassPhrase, int EncryptType)
{
    TEncryptConfig *Config;

    Config=EncryptGetConfig(EncryptType);
    if (Config)
    {
        //generate a path that we will write the encrypted file to
        RetStr=EncryptFileGetPath(RetStr, Path);
        RunExternalCommand(Config->EncryptCommand, Config->PassPrompt, PassPhrase, Path, RetStr);
        EncryptConfigDestroy(Config);
    }

    return(RetStr);
}


char *DecryptFile(char *RetStr, const char *Path, const char *PassPhrase, int EncryptType)
{
    TEncryptConfig *Config;

    RetStr=CopyStr(RetStr, Path);
    StrRTruncChar(RetStr, '.');
    Config=DecryptGetConfig(Path, EncryptType);
    if (Config)
    {
        RunExternalCommand(Config->DecryptCommand, Config->PassPrompt, PassPhrase, Path, RetStr);
        EncryptConfigDestroy(Config);
    }

    return(RetStr);
}


