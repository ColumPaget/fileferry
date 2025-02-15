#include "ui.h"
#include "file_transfer.h"
#include "filecache.h"
#include "stdout.h"
#include "image_display.h"
#include "file_include_exclude.h"
#include "fileitem.h"
#include "filestore.h"
#include "filestore_dirlist.h"
#include "commands.h"
#include "settings.h"
#include <stdarg.h>

STREAM *StdIO=NULL;


void UI_Init()
{
    StdIO=STREAMFromDualFD(0,1);
    TerminalInit(StdIO, TERM_RAWKEYS | TERM_SAVEATTRIBS);
}

void UI_Close()
{
    TerminalReset(StdIO);
}

void UI_Exit(int RetVal)
{
    TerminalReset(StdIO);
		exit(RetVal);
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



/*
#define SETTING_PROGRESS 4
#define SETTING_TIMESTAMPS 8
*/


static void UI_ShowFlag(const char *Name, int Flag)
{
    if (Settings->Flags & Flag) printf("%-20s yes\n", Name);
    else printf("%-20s no\n", Name);
}


void UI_ShowSettings()
{
    const char *ptr;

    UI_ShowFlag("verbose", SETTING_VERBOSE);
    UI_ShowFlag("debug",   SETTING_DEBUG);
    UI_ShowFlag("syslog",  SETTING_SYSLOG);
    UI_ShowFlag("sixel",   SETTING_SIXEL);
    UI_ShowFlag("nols",    SETTING_NO_DIR_LIST);
    UI_ShowFlag("progress",SETTING_PROGRESS);
    UI_ShowFlag("integrity",SETTING_INTEGRITY_CHECK);
    UI_ShowFlag("xterm-title",SETTING_XTERM_TITLE);

    if (Settings->Flags & SETTING_LIST_LONG) ptr="long";
    else if (Settings->Flags & SETTING_LIST_LONG_LONG) ptr="full";
    else ptr="basic";
    printf("%-20s %s\n", "list-type", ptr);

    printf("%-20s %s\n", "config-file", Settings->ConfigFile);
    printf("%-20s %s\n", "proxy", Settings->ProxyChain);
    printf("%-20s %d\n", "life", Settings->ProcessTimeout);
    printf("%-20s %d\n", "timeout", Settings->CommandTimeout);
    printf("%-20s %s\n", "log-file", Settings->LogFile);
    printf("%-20s %s\n", "errors-email", Settings->EmailForErrors);
    printf("%-20s %s\n", "mail-sender", Settings->EmailSender);
    printf("%-20s %s\n", "smtp-server", Settings->SmtpServer);
    printf("%-20s %s\n", "image-size", Settings->ImagePreviewSize);
    printf("%-20s %s\n", "viewers", Settings->ImageViewers);
    printf("%-20s %s\n", "sixelers", Settings->Sixelers);


    fflush(NULL);
}



static int UI_ListFlags(TCommand *Cmd)
{
    if (Settings->Flags & SETTING_LIST_LONG_LONG) return(CMD_FLAG_LONG_LONG);
    if (Settings->Flags & SETTING_LIST_LONG) return(CMD_FLAG_LONG);

    return(Cmd->Flags);
}


void UI_OutputDirList(TFileStore *FS, TCommand *Cmd)
{
    ListNode *DirList, *Curr;
    TFileItem *FI;
    const char *prefix="", *class="", *whenstr="", *sizestr="";
    char *Tempstr=NULL;
    int LineCount=0;
    int ListFlags=0;

    time(&Now);

    //do this to handle Cmd->Target being NULL
    Tempstr=CopyStr(Tempstr, Cmd->Target);

    if (Cmd->Flags & CMD_FLAG_FORCE) DirList=FileStoreReloadAndGlob(FS, Tempstr);
    else DirList=FileStoreGlob(FS, Tempstr);

    if (DirList)
    {
        if (Cmd->Flags & CMD_FLAG_SORT_SIZE) ListSort(DirList, NULL, SortSizeCompare);
        if (Cmd->Flags & CMD_FLAG_SORT_TIME) ListSort(DirList, NULL, SortTimeCompare);
    }

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

                ListFlags=UI_ListFlags(Cmd);

                if (ListFlags & (CMD_FLAG_LONG | CMD_FLAG_LONG_LONG))
                {
                    if (ListFlags & CMD_FLAG_LONG_LONG) whenstr=GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", FI->mtime, NULL);
                    else if ((Now - FI->mtime) < (3600 * 24)) whenstr=GetDateStrFromSecs("%H:%M:%S", FI->mtime, NULL);
                    else whenstr=GetDateStrFromSecs("%Y/%m/%d", FI->mtime, NULL);

                    if (ListFlags & CMD_FLAG_LONG_LONG) Tempstr=FormatStr(Tempstr, "% 12llu % 10s % 10s %s% 30s%s~0", (unsigned long long) FI->size, whenstr, FI->user, prefix, FI->name, class);
                    else Tempstr=FormatStr(Tempstr, "% 8s % 10s % 10s %s% 30s%s~0", ToMetric((double)FI->size, 1), whenstr, FI->user, prefix, FI->name, class);

                    if (StrValid(FI->hash)) Tempstr=MCatStr(Tempstr, "	", FI->hash, NULL);
                    if (StrValid(FI->destination)) Tempstr=MCatStr(Tempstr, " -> ", FI->destination, NULL);
                    if (StrValid(FI->title)) Tempstr=MCatStr(Tempstr, "  ", FI->title, "", NULL);

                    if (ListFlags & CMD_FLAG_LONG_LONG)
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
                    printf("enter 'quit' to end, 'resume' to cease paging, anything else for next page\r");
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


    FileStoreDirListFree(FS, DirList);

    Destroy(Tempstr);
}


void UI_OutputFStat(TFileStore *FS, TCommand *Cmd)
{
    ListNode *DirList, *Curr;
    unsigned int Files=0, Dirs=0;
    char *Tempstr=NULL;
    TFileItem *FI;
    double Bytes=0, Largest=0;
    time_t Oldest=0, Newest=0;

//do this to handle Cmd->Target being NULL
    Tempstr=CopyStr(Tempstr, Cmd->Target);

    if (Cmd->Flags & CMD_FLAG_FORCE) DirList=FileStoreReloadAndGlob(FS, Tempstr);
    else DirList=FileStoreGlob(FS, Tempstr);

    Curr=ListGetNext(DirList);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FileIncluded(Cmd, FI, FS, FS))
        {
            if (FI->type==FTYPE_DIR) Dirs++;
            else
            {
                Files++;
                Bytes+=FI->size;
                if (FI->size > Largest) Largest=FI->size;
                if (FI->mtime > Newest) Newest=FI->mtime;
                if ((Oldest==0) || (FI->mtime < Oldest)) Oldest=FI->mtime;
            }
        }
        Curr=ListGetNext(Curr);
    }

    printf("dirs:%d files:%d total:%sb ", Dirs, Files, ToIEC(Bytes, 1));
    printf("largest:%sb ", ToIEC(Largest, 1));
    printf("oldest:%s ", GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", Oldest, NULL));
    printf("newest:%s\n", GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", Newest, NULL));

    FileStoreDirListFree(FS, DirList);
    Destroy(Tempstr);
}



void UI_DisplayDiskSpace(double total, double used, double avail)
{
    char *Tempstr=NULL, *Value=NULL;

    Tempstr=MCopyStr(Tempstr, "Disk Space: total: ", ToIEC(total, 2), NULL);
    Value=FormatStr(Value, " used: %0.1f%% (%sb) ", used * 100.0 / total, ToIEC(used, 2));
    Tempstr=CatStr(Tempstr, Value);
    Value=FormatStr(Value, " avail: %0.1f%% (%sb) ", avail * 100.0 / total, ToIEC(avail, 2));
    Tempstr=CatStr(Tempstr, Value);
    UI_Output(0, "%s", Tempstr);

    Destroy(Tempstr);
    Destroy(Value);
}




TCommand *UI_ReadCommand(TFileStore *FS)
{
    char *Tempstr=NULL, *Input=NULL;
    TCommand *Cmd;


    if (Settings->Flags & SETTING_XTERM_TITLE)
    {
        Tempstr=MCopyStr(Tempstr, "fileferry: ", FS->Name, NULL);
        XtermSetTitle(StdIO, Tempstr);
    }

    STREAMSetTimeout(StdIO, 0);
    Tempstr=MCopyStr(Tempstr, FS->URL, ": ", NULL);

#ifdef HAVE_PROMPT_HISTORY
    static ListNode *History=NULL;

    if (! History) History=ListCreate();
    Input=TerminalReadPromptWithHistory(Input, Tempstr, 0, History, StdIO);
#else
    Input=TerminalReadPrompt(Input, Tempstr, 0, StdIO);
#endif

    if (StrValid(Input))
    {
    STREAMWriteLine("\n", StdIO);
    StripTrailingWhitespace(Input);
    if (StrValid(Input)) Cmd=CommandParse(Input);
    }

    Destroy(Tempstr);
    Destroy(Input);
    return(Cmd);
}


int UI_TransferProgress(TFileTransfer *Xfer)
{
    double percent;
    char *BPS=NULL, *Size=NULL, *Transferred=NULL, *Tempstr=NULL, *XofX=NULL;
    static uint64_t LastCentisecs=0;
    uint64_t Centisecs=0, diff;

    if (isatty(0))
    {
        Centisecs=GetTime(TIME_CENTISECS);
        diff=((long) (Centisecs)) - Xfer->StartTime;
        if ( ((Centisecs - LastCentisecs) > 20) || (Xfer->Offset == Xfer->Size) )
        {
            if (diff > 0) BPS=CopyStr(BPS, ToMetric((float) Xfer->Downloaded / ((float)diff / 100.0), 1));
            else BPS=CopyStr(BPS, "0.0");

            if (Xfer->TotalFiles > 1) XofX=FormatStr(XofX, "%llu/%llu ", Xfer->CurrFileNum+1, Xfer->TotalFiles);
            else XofX=CopyStr(XofX, "");

            if (Xfer->Size > 0)
            {
                percent=(double) Xfer->Offset * 100.0 / (double) Xfer->Size;
                Size=CopyStr(Size, ToMetric((double) Xfer->Size, 1));
                Transferred=CopyStr(Transferred, ToMetric((double) Xfer->Offset, 1));
                printf("%s% 5.1f%% %s  (%s of %s) %sbps         \r", XofX, percent, Xfer->DestFinalName, Transferred, Size, BPS);
            }
            else printf("%s%s %s %sbps            \r", XofX, Transferred, Xfer->DestFinalName, BPS);

            if (Settings->Flags & SETTING_XTERM_TITLE)
            {
                if (Xfer->Size > 0) Tempstr=FormatStr(Tempstr, "%s%0.1f%%  %s", XofX, percent, Xfer->DestFinalName);
                else Tempstr=FormatStr(Tempstr, "%s%s  %s", XofX, Transferred, Xfer->DestFinalName);

                XtermSetTitle(StdIO, Tempstr);
            }

            fflush(NULL);
            LastCentisecs=Centisecs;
        }
    }

    Destroy(Tempstr);
    Destroy(Transferred);
    Destroy(BPS);
    Destroy(Size);

    return(TRUE);
}


static void UI_ShowFilePostProcess(TCommand *Cmd, TFileTransfer *Xfer)
{
    char *Tempstr=NULL;

    Tempstr=FileStoreReformatPath(Tempstr, Xfer->DestFinalName, Xfer->ToFS);
    if (Cmd->Flags & CMD_FLAG_IMG) DisplayImage(Cmd, Xfer->SourceFinalName, Tempstr);
    else unlink(Tempstr);

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

    FileStoreDirListFree(FromFS, DirList);

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
        if (Flags & UI_OUTPUT_ERROR) TerminalPrint(StdIO, "ERROR: %s\n", Tempstr);
        else if (Flags & UI_OUTPUT_SUCCESS) TerminalPrint(StdIO, "%s\n", Tempstr);
        else TerminalPrint(StdIO, "%s\n", Tempstr);
        va_end(list);
    }

    Destroy(Tempstr);
}



void UI_MainLoop(TFileStore *LocalFS, TFileStore *RemoteFS)
{
    TCommand *Cmd;
    const char *ptr;

    Cmd=UI_ReadCommand(RemoteFS);
    while (Cmd->Type != CMD_QUIT)
    {
        CommandProcess(Cmd, LocalFS, RemoteFS);
        CommandDestroy(Cmd);

        Cmd=UI_ReadCommand(RemoteFS);
    }

    CommandDestroy(Cmd);
}


