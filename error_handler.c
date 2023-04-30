#include "error_handler.h"
#include "settings.h"
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

void HandleError(TFileStore *FS, int ErrorType, const char *Path, const char *Description)
{
    if (Settings->Flags & SETTING_SYSLOG) syslog(LOG_ERR, "%s: path=%s host=%s", Description, Path, FS->URL);
    if (StrLen(Settings->EmailForErrors)) EmailError(FS, ErrorType, Path, Description);
//if (StrLen(Settings->WebhookForErrors)) WebhookError(FS, ErrorType, Path, Description);
}
