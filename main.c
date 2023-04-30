#include "common.h"
#include "fileitem.h"
#include "filestore.h"
#include "filestore_drivers.h"
#include "ui.h"
#include "settings.h"
#include "commands.h"
#include "errors_and_logging.h"
#include "proc.h"

int SigHandler(int Sig)
{
    if (Sig==SIGALRM) exit(1);
}


int ApplicationInit(int argc, const char *argv[])
{
    int Act;
    char *Tempstr=NULL;

    ProcessStartTime=GetTime(0);
    FileStoreDriversInit();
    UI_Init();
    Act=SettingsInit(argc, argv);
    LibUsefulSetValue("HTTP:UserAgent", "fileferry-2.0");
    LogInit();

    Tempstr=ProcLookupParent(Tempstr);
    if (Settings->LogFile) HandleEvent(NULL, 0, "Starting up. parent=$(path)", Tempstr, "");

    Destroy(Tempstr);
    return(Act);
}


void HandleConnection(TFileStore *FS)
{
    char *Tempstr=NULL, *Proxy=NULL;

    if (StrValid(Settings->ProxyChain)) Proxy=CopyStr(Proxy, Settings->ProxyChain);
    if (StrValid(FS->Proxy)) Proxy=CopyStr(Proxy, FS->Proxy);
    if (StrValid(Proxy)) Tempstr=FormatStr(Tempstr, "Connected to %s using proxy %s. time is %d seconds adrift from local", FS->URL, Proxy, FS->TimeOffset);
    else Tempstr=FormatStr(Tempstr, "Connected to %s. time is %d seconds adrift from local", FS->URL, FS->TimeOffset);
    HandleEvent(FS, 0, Tempstr, "", "");

    Destroy(Tempstr);
    Destroy(Proxy);
}


int main(int argc, const char *argv[])
{
    TFileStore *RemoteFS, *LocalFS;
    char *Tempstr=NULL;
    uint64_t offset=0;
    uint32_t len, result, i;
    STREAM *S;

    if (ApplicationInit(argc, argv))
    {
        Tempstr=MCopyStr(Tempstr, "remote ", Settings->URL, NULL);
        if (StrValid(Settings->User)) Tempstr=MCatStr(Tempstr, " user='", Settings->User, "'", NULL);
        if (StrValid(Settings->Pass)) Tempstr=MCatStr(Tempstr, " pass='", Settings->Pass, "'", NULL);
        if (StrValid(Settings->Encryption)) Tempstr=MCatStr(Tempstr, " encrypt='", Settings->Encryption, "'", NULL);
        if (StrValid(Settings->IdentityFile)) Tempstr=MCatStr(Tempstr, " identityfile='", Settings->IdentityFile, "'", NULL);

        if (Settings->ProcessTimeout > 0) alarm(Settings->ProcessTimeout);

        LocalFS=FileStoreConnect("local file:.");
        RemoteFS=FileStoreConnect(Tempstr);

        if (! RemoteFS)
        {
            fprintf(stderr, "ERROR: unknown filestore: '%s'\n", Tempstr);
            exit(1);
        }

        if (! (RemoteFS->Flags & FILESTORE_ATTACHED))
        {
            fprintf(stderr, "ERROR: failed to find driver for %s\n", Settings->URL);
            exit(1);
        }

        if (! (RemoteFS->Flags & FILESTORE_CONNECTED))
        {
            fprintf(stderr, "ERROR: failed to connect to %s\n", Settings->URL);
            exit(1);
        }


        HandleConnection(RemoteFS);

        if (StrValid(Settings->Commands)) CommandListProcess(Settings->Commands, LocalFS, RemoteFS);
        else UI_MainLoop(LocalFS, RemoteFS);

        FileStoreDisConnect(RemoteFS);
    }

    if (StrValid(Settings->LogFile)) LogFileClose(Settings->LogFile);

    Destroy(Tempstr);
}
