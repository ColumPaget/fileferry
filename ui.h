
#ifndef FILEFERRY_UI_H
#define FILEFERRY_UI_H

#include "common.h"
#include "filestore.h"
#include "file_transfer.h"

#define UI_OUTPUT_VERBOSE 1
#define UI_OUTPUT_ERROR 2
#define UI_OUTPUT_SUCCESS 4
#define UI_OUTPUT_DEBUG 8

void UI_Init();
void UI_Close();
void UI_MainLoop(TFileStore *LocalFS, TFileStore *RemoteFS);
char *UI_AskPassword(char *Password);
int UI_TransferProgress(TFileTransfer *Xfer);
void UI_Output(int Flags, const char *Fmt, ...);
void UI_OutputDirList(TFileStore *FS, TCommand *Cmd);
void UI_OutputFStat(TFileStore *FS, TCommand *Cmd);
void UI_DisplayDiskSpace(double total, double used, double avail);
void UI_ShowSettings();
int UI_ShowFile(TFileStore *FromFS, TFileStore *LocalFS, TCommand *Cmd);
void UI_Exit(int RetVal);

#endif
