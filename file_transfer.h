#ifndef FILEFERRY_FILE_TRANSFER_H
#define FILEFERRY_FILE_TRANSFER_H

#include "common.h"
#include "filestore.h"
#include "fileitem.h"
#include "commands.h"

#define XFER_OKAY 0
#define XFER_SOURCE_FAIL 1
#define XFER_DEST_FAIL   2
#define XFER_SHORT_FAIL  3
#define XFER_DEST_EXISTS 4

#define XFER_FLAG_DOWNLOAD 1
#define XFER_FLAG_UPLOAD   2
#define XFER_FLAG_RECURSE  4
#define XFER_FLAG_TMPNAME  8
#define XFER_FLAG_FORCE   16


typedef struct t_file_transfer TFileTransfer;

typedef int(*PROGRESS_FUNC)(struct t_file_transfer *Xfer);
struct t_file_transfer
{
    int Flags;
    uint64_t Size;
    uint64_t Offset;
    uint64_t Downloaded;
    time_t StartTime;
    int DestBackups;
    int EncryptType;
    char *EncryptKey;
    char *Path;
    char *PreProcessedPath;
    char *TransferName;
    char *SourceFinalName;
    char *DestFinalName;
    char *SourceFinalExtn;
    char *DestFinalExtn;
    char *SourceTempExtn;
    char *DestTempExtn;
    char *PreHookScript;
    char *PostHookScript;
    TFileStore *FromFS;
    TFileStore *ToFS;
    PROGRESS_FUNC ProgressFunc;
};


TFileTransfer *FileTransferFromCommand(TCommand *Cmd, TFileStore *FromFS, TFileStore *ToFS, TFileItem *FI);
void FileTransferDestroy(void *p_Xfer);
int TransferNeeded(TCommand *Cmd, TFileItem *FI, TFileStore *ToFS);
int TransferFile(TFileTransfer *Xfer);
int TransferFileCommand(TFileStore *FromFS, TFileStore *ToFS, TCommand *Cmd);

#endif


