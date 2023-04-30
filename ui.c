#include "ui.h"
#include "file_transfer.h"
#include "filecache.h"
#include "stdout.h"
#include "image_display.h"
#include "file_include_exclude.h"
#include "fileitem.h"
#include "commands.h"
#include "settings.h"
#include <stdarg.h>

STREAM *StdIO=NULL;


void UI_Init()
{
    StdIO=STREAMFromDualFD(0,1);
    TerminalInit(StdIO, 0);
}

void UI_Close()
{
    TerminalReset(StdIO);
}

int SortSizeCompare(void *data, void *i1, void *i2)
{
    TFileItem *f1, *f2;

    f1=(TFileItem *) i1;
    f2=(TFileItem *) i2;
    if (f1->size < f2->size) return(TRUE);
    return(FALSE);
}

int SortTimeCompare(void *data, void *i1, void *i2)
{
    TFileItem *f1, *f2;

    f1=(TFileItem *) i1;
    f2=(TFileItem *) i2;
    if (f1->mtime < f2->mtime) return(TRUE);
    return(FALSE);
}


void UI_ShowSettings()
{
		if (Settings->Flags & SETTING_VERBOSE) printf("verbose:    yes\n");
		else printf("verbose:    no\n");
		if (Settings->Flags & SETTING_DEBUG)   printf("debug:      yes\n");
		else printf("debug:      no\n");
		if (Settings->Flags & SETTING_SYSLOG)  printf("syslog:     yes\n");
		else printf("syslog:     no\n");
    printf("proxy:      %s\n", Settings->ProxyChain);
    printf("log-file:   %s\n", Settings->LogFile);
    printf("timeout:    %d\n", Settings->CommandTimeout);
    printf("image-size: %s\n", Settings->ImagePreviewSize);
    printf("viewers:    %s\n", Settings->ImageViewers);
		if (Settings->Flags & SETTING_SIXEL) printf("sixel:      yes\n");
		else printf("sixel:      no\n");
    printf("sixelers:   %s\n", Settings->Sixelers);
    fflush(NULL);
// else if (strcmp(Name, "smtp-server")==0) Settings->SmtpServer=CopyStr(Settings->SmtpServer, Value);
// else if (strcmp(Name, "errors-email")==0) Settings->EmailForErrors=CopyStr(Settings->EmailForErrors, Value);
}


void UI_OutputDirList(TFileStore *FS, TCommand *Cmd)
{
    ListNode *DirList, *Curr;
    TFileItem *FI;
    const char *prefix="", *class="", *whenstr="";
    char *Tempstr=NULL;
    int LineCount=0;

    time(&Now);
    //do this to handle Cmd->Target being NULL
    Tempstr=CopyStr(Tempstr, Cmd->Target);
    DirList=FileStoreGlob(FS, Tempstr);

    if (Cmd->Flags & CMD_FLAG_SORT_SIZE) ListSort(DirList, NULL, SortSizeCompare);
    if (Cmd->Flags & CMD_FLAG_SORT_TIME) ListSort(DirList, NULL, SortTimeCompare);

    Curr=ListGetNext(DirList);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FileIncluded(Cmd, FI, FS, FS))
        {
            if ( (FI->name[0] != '.') || (Cmd->Flags & CMD_FLAG_ALL) )
            {
                prefix="~e";
                class="";
                switch (FI->type)
                {
                case FTYPE_FILE:
                    if (FI->perms & (S_IXUSR | S_IXGRP)) class="@";
                    break;
                case FTYPE_DIR:
                    prefix="~b";
                    class="/";
                    break;
                case FTYPE_FIFO:
                    prefix="~r";
                    class="|";
                    break;
                case FTYPE_SOCKET:
                    prefix="~r";
                    class="=";
                    break;
                }

                if (Cmd->Flags & CMD_FLAG_LONG)
                {
                    if (Cmd->Flags & CMD_FLAG_LONG_LONG) whenstr=GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", FI->mtime, NULL);
                    else if ((Now - FI->mtime) < (3600 * 24)) whenstr=GetDateStrFromSecs("%H:%M:%S", FI->mtime, NULL);
                    else whenstr=GetDateStrFromSecs("%Y/%m/%d", FI->mtime, NULL);

                    Tempstr=FormatStr(Tempstr, "% 8s % 10s % 10s %s% 30s%s~0", ToMetric((double)FI->size, 1), whenstr, FI->user, prefix, FI->name, class);
                    if (StrValid(FI->hash)) Tempstr=MCatStr(Tempstr, "	", FI->hash, NULL);
                    if (StrValid(FI->destination)) Tempstr=MCatStr(Tempstr, " -> ", FI->destination, NULL);
                    if (StrValid(FI->title)) Tempstr=MCatStr(Tempstr, "  ", FI->title, "", NULL);

                    if (Cmd->Flags & CMD_FLAG_LONG_LONG)
                    {
                        if (StrValid(FI->description)) Tempstr=MCatStr(Tempstr, "\n  ", FI->description, "\n", NULL);
                    }
                }
                else Tempstr=MCopyStr(Tempstr, FI->name, class, NULL);
                Tempstr=CatStr(Tempstr, "\n");

                TerminalPutStr(Tempstr, StdIO);
                LineCount++;
            }

            if ((Cmd->NoOfItems > 0) && (LineCount > Cmd->NoOfItems)) break;

            if (Cmd->Flags & CMD_FLAG_PAGE)
            {
                if (LineCount > 10)
                {
                    printf("--- PRESS ENTER FOR MORE ---\r");
                    fflush(NULL);
                    Tempstr=STREAMReadLine(Tempstr, StdIO);
                    StripTrailingWhitespace(Tempstr);
                    if (strcmp(Tempstr, "quit")==0) break;
                    if (strcmp(Tempstr, "resume")==0) Cmd->Flags &= ~CMD_FLAG_PAGE;
                    LineCount=0;
                }
            }
        }

        Curr=ListGetNext(Curr);
    }


    FileStoreFreeDir(FS, DirList);

    Destroy(Tempstr);
}


void UI_DisplayPrompt(TFileStore *FS)
{
    char *Tempstr=NULL;

    Tempstr=MCopyStr(Tempstr, "\r", FS->URL, ": ", NULL);
    STREAMWriteLine(Tempstr, StdIO);
    STREAMFlush(StdIO);

    Destroy(Tempstr);
}



TCommand *UI_ReadCommand(TFileStore *FS)
{
    char *Tempstr=NULL;
    TCommand *Cmd;

    STREAMSetTimeout(StdIO, 0);
    UI_DisplayPrompt(FS);
    Tempstr=STREAMReadLine(Tempstr, StdIO);
    StripTrailingWhitespace(Tempstr);
    if (StrValid(Tempstr)) Cmd=CommandParse(Tempstr);

    Destroy(Tempstr);
    return(Cmd);
}


int UI_TransferProgress(TFileTransfer *Xfer)
{
    double percent;
    char *BPS=NULL, *Size=NULL;
    long secs;

    if (isatty(0))
    {
        time(&Now);
        secs=Now-Xfer->StartTime;
        if (secs > 0) BPS=CopyStr(BPS, ToMetric(Xfer->Downloaded / secs, 1));
        else BPS=CopyStr(BPS, "0.0");

        if (Xfer->Size > 0)
        {
            percent=(double) Xfer->Offset * 100.0 / (double) Xfer->Size;
            Size=CopyStr(Size, ToMetric((double) Xfer->Size, 1));
            printf("% 5.1f%% %s  (%s of %s) %sbps         \r", percent, Xfer->DestFinalName, ToMetric((double) Xfer->Offset, 1), Size, BPS);
        }
        else printf("%s %s %sbps            \r", ToMetric((double) Xfer->Offset, 1), Xfer->DestFinalName, BPS);
    }

    Destroy(BPS);
    Destroy(Size);

    return(TRUE);
}


static void UI_ShowFilePostProcess(TCommand *Cmd, TFileTransfer *Xfer)
{
    char *Tempstr=NULL;

    Tempstr=FileStoreReformatPath(Tempstr, Xfer->DestFinalName, Xfer->ToFS);
    if (Cmd->Flags & CMD_FLAG_IMG) DisplayImage(Cmd, Xfer->SourceFinalName, Tempstr);
    Destroy(Tempstr);
}


int UI_ShowFile(TFileStore *FromFS, TFileStore *LocalFS, TCommand *Cmd)
{
    TFileTransfer *Xfer;
    TFileItem *FI;
    ListNode *DirList, *Curr;
    TFileStore *StdOutFS, *CacheFS, *ToFS;
    int result;

    StdOutFS=STDOUTFilestoreCreate();
    CacheFS=FileCacheCreate();

    DirList=FileStoreGlob(FromFS, Cmd->Target);
    Curr=ListGetNext(DirList);
    if (! Curr) printf("ERROR: No files matching '%s'\n", Cmd->Target);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FileIncluded(Cmd, FI, FromFS, StdOutFS))
        {
            if (Cmd->Flags & CMD_FLAG_IMG) ToFS=CacheFS;
            else ToFS=StdOutFS;
            Xfer=FileTransferFromCommand(Cmd, FromFS, ToFS, FI);
            result=TransferFile(Xfer);
            if (result==XFER_SOURCE_FAIL) printf("ERROR: Failed to open source file %s\n", Xfer->Path);
            else if (result==XFER_DEST_FAIL) printf("ERROR: Failed to open destination file %s\n", Xfer->Path);
            else UI_ShowFilePostProcess(Cmd, Xfer);
            FileTransferDestroy(Xfer);
        }

        Curr=ListGetNext(Curr);
    }

    FileStoreFreeDir(FromFS, DirList);

    FileStoreDestroy(StdOutFS);
    FileStoreDestroy(CacheFS);

    return(TRUE);
}



char *UI_AskPassword(char *Password)
{
    if (isatty(0))
    {
        Password=CopyStr(Password, "");
        Password=TerminalReadPrompt(Password, "~ePassword:~0 ", TERM_SHOWTEXTSTARS, StdIO);
        UI_Output(0, "");
    }
    return(Password);
}


static int UI_OutputRequired(int Flags)
{
    if ( (Flags & UI_OUTPUT_VERBOSE) && (! (Settings->Flags & SETTING_VERBOSE)) ) return(FALSE);
    if ( (Flags & UI_OUTPUT_DEBUG) && (! (Settings->Flags & SETTING_DEBUG)) ) return(FALSE);

    return(TRUE);
}


void UI_Output(int Flags, const char *Fmt, ...)
{
    char *Tempstr=NULL;
    va_list list;

    if (UI_OutputRequired(Flags))
    {
        //if (Settings->Flags & SETTING_TIMESTAMPS) printf("%s ", GetDateStr("%Y/%m/%d %H:%M:%S", NULL));
        va_start(list, Fmt);
        Tempstr=VFormatStr(Tempstr, Fmt, list);
        if (Flags & UI_OUTPUT_ERROR) printf("ERROR: %s\n", Tempstr);
        else if (Flags & UI_OUTPUT_SUCCESS) printf("%s\n", Tempstr);
        else printf("%s\n", Tempstr);
        va_end(list);
    }

    Destroy(Tempstr);
}



void UI_MainLoop(TFileStore *LocalFS, TFileStore *RemoteFS)
{
    TCommand *Cmd;
    const char *ptr;

    if (StrValid(RemoteFS->Features) && (Settings->Flags & SETTING_VERBOSE)) UI_Output(0, "Features: %s\n", RemoteFS->Features);
    ptr=GetVar(RemoteFS->Vars, "LoginBanner");
    if (StrValid(ptr)) printf("%s\n", ptr);

    Cmd=UI_ReadCommand(RemoteFS);
    while (Cmd->Type != CMD_QUIT)
    {
        CommandProcess(Cmd, LocalFS, RemoteFS);
        CommandDestroy(Cmd);
        Cmd=UI_ReadCommand(RemoteFS);
    }

    CommandDestroy(Cmd);
}


