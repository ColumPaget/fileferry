#include "errors_and_logging.h"
#include "settings.h"
#include "ui.h"
#include <syslog.h>

void EmailError(TFileStore *FS, int ErrorType, const char *Path, const char *Description)
{
    char *Tempstr=NULL, *Sender=NULL;

    if (StrValid(Settings->EmailSender)) Sender=CopyStr(Sender, Settings->EmailSender);
    else
    {
        Tempstr=SetStrLen(Tempstr, 1024);
        gethostname(Tempstr, 1024);
        Sender=MCopyStr(Sender, "fileferry@", Tempstr, NULL);
    }

    Tempstr=FormatStr(Tempstr, "%s: path=%s host=%s", Description, Path, FS->URL);
    SMTPSendMail(Settings->EmailSender, Settings->EmailForErrors, Tempstr, Tempstr, 0);

    Destroy(Tempstr);
    Destroy(Sender);
}


void HandleEvent(TFileStore *FS, int ErrorType, const char *Description, const char *Path, const char *Dest)
{
    char *Tempstr=NULL, *Cleaned=NULL;
    ListNode *Vars=NULL;

    if (! (Settings->Flags & SETTING_NOERROR))
    {
        Vars=ListCreate();
        SetVar(Vars, "path", Path);
        SetVar(Vars, "dest", Dest);
        if (FS) SetVar(Vars, "filestore", FS->URL);

        Tempstr=SubstituteVarsInString(Tempstr, Description, Vars, 0);
        UI_Output(ErrorType, "%s", Tempstr);

				Cleaned=TerminalStripControlSequences(Cleaned, Tempstr);
        if (StrValid(Settings->LogFile)) LogToFile(Settings->LogFile, "%s", Cleaned);
        if (Settings->Flags & SETTING_SYSLOG) syslog(LOG_ERR, "%s", Cleaned);
        if (StrLen(Settings->EmailForErrors)) EmailError(FS, ErrorType, Path, Description);
//if (StrLen(Settings->WebhookForErrors)) WebhookError(FS, ErrorType, Path, Description);
    }

    ListDestroy(Vars, Destroy);

    Destroy(Tempstr);
    Destroy(Cleaned);
}


void LogInit()
{
    if (StrValid(Settings->LogFile)) LogFileFindSetValues(Settings->LogFile, LOGFILE_LOGPID | LOGFILE_TIMESTAMP, Settings->LogMaxSize, 5, 0);
}


int SendLoggedLine(const char *Line, TFileStore *FS, STREAM *S)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    int result;

    result=STREAMWriteLine(Line, S);
    STREAMFlush(S);

    ptr=GetToken(Line, "\n", &Token, 0);
    while (ptr)
    {
        Tempstr=MCatStr(Tempstr, "C>>S ", Token, "\n", NULL);
        ptr=GetToken(ptr, "\n", &Token, 0);
    }
    StripCRLF(Tempstr);
    HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, S->Path, "");

    Destroy(Tempstr);
    Destroy(Token);

    return(result);
}


char *ReadLoggedLine(char *Line, TFileStore *FS,  STREAM *S)
{
    char *Tempstr=NULL;

    Line=STREAMReadLine(Line, S);
    StripCRLF(Line);
    Tempstr=MCopyStr(Tempstr, "C<<S ", Line, NULL);
    HandleEvent(FS, UI_OUTPUT_DEBUG, Tempstr, S->Path, "");

    Destroy(Tempstr);

    return(Line);
}

