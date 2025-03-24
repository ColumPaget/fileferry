#include "file_transfer.h"
#include "fileitem.h"
#include "encrypt.h"
#include "ui.h"
#include "settings.h"
#include "file_include_exclude.h"
#include "errors_and_logging.h"
#include "filestore.h"
#include "filestore_dirlist.h"



void FileTransferDestroy(void *p_Xfer)
{
    TFileTransfer *Xfer;

    Xfer=(TFileTransfer *) p_Xfer;

    Destroy(Xfer->Path);
    Destroy(Xfer->PreProcessedPath);
    Destroy(Xfer->TransferName);
    Destroy(Xfer->SourceFinalName);
    Destroy(Xfer->DestFinalName);
    Destroy(Xfer->EncryptKey);
    Destroy(Xfer->SourceFinalExtn);
    Destroy(Xfer->DestFinalExtn);
    Destroy(Xfer->SourceTempExtn);
    Destroy(Xfer->DestTempExtn);
    Destroy(Xfer->PreHookScript);
    Destroy(Xfer->PostHookScript);
    Destroy(Xfer);
}






TFileTransfer *FileTransferFromCommand(TCommand *Cmd, TFileStore *FromFS, TFileStore *ToFS, TFileItem *FI)
{
    TFileTransfer *Xfer;
    const char *p_Var, *p_DestName;
    TFileItem *DestFI;

    Xfer=(TFileTransfer *) calloc(1, sizeof(TFileTransfer));

    Xfer->EncryptType = Cmd->EncryptType;
    if (Cmd->Flags & CMD_FLAG_FORCE) Xfer->Flags |= XFER_FLAG_FORCE;
    if (Cmd->Flags & CMD_FLAG_THUMB) Xfer->Flags |= XFER_FLAG_THUMB;

    if (
        (Cmd->Flags & CMD_FLAG_RESUME) &&
        (FromFS->Flags & FILESTORE_RESUME_TRANSFERS) &&
        (ToFS->Flags & FILESTORE_RESUME_TRANSFERS)
    )
    {
        DestFI=FileStoreGetFileInfo(ToFS, FI->name);
        if (DestFI)
        {
            Xfer->Offset=DestFI->size;
            Xfer->Flags |= XFER_FLAG_RESUME;
        }
    }



    if (Cmd->Type==CMD_PUT)
    {
        Xfer->Flags |= XFER_FLAG_UPLOAD;

        p_Var=ToFS->Encrypt;
        if (StrValid(Cmd->EncryptKey)) p_Var=Cmd->EncryptKey;
        if (StrValid(p_Var)) Xfer->EncryptKey=CopyStr(Xfer->EncryptKey, p_Var);
    }
    else
    {
        Xfer->Flags |= XFER_FLAG_DOWNLOAD;

        p_Var=FromFS->Encrypt;
        if (StrValid(Cmd->EncryptKey)) p_Var=Cmd->EncryptKey;
        if (StrValid(p_Var)) Xfer->EncryptKey=CopyStr(Xfer->EncryptKey, p_Var);
    }

    Xfer->FromFS=FromFS;
    Xfer->ToFS=ToFS;
    Xfer->Path=CopyStr(Xfer->Path, FI->path);
    Xfer->Size=FI->size;
    if (StrValid(FI->name)) p_DestName=FI->name;
    else p_DestName=GetBasename(Xfer->Path);

    Xfer->TransferName=MCopyStr(Xfer->TransferName, p_DestName, NULL);
    Xfer->DestFinalName=CopyStr(Xfer->DestFinalName, p_DestName);

    p_Var=GetVar(Cmd->Vars, "DestTransferExtn");
    if (StrValid(p_Var))
    {
        Xfer->DestFinalName=CopyStr(Xfer->DestFinalName, p_DestName);
        Xfer->TransferName=MCopyStr(Xfer->TransferName, Xfer->DestFinalName, p_Var, NULL);
    }

    p_Var=GetVar(Cmd->Vars, "DestFinalExtn");
    if (StrValid(p_Var)) Xfer->DestFinalName=CopyPathChangeExtn(Xfer->DestFinalName, p_DestName, p_Var, EXTN_CHANGE);
    else
    {
        p_Var=GetVar(Cmd->Vars, "DestAppendExtn");
        if (StrValid(p_Var)) Xfer->DestFinalName=CopyPathChangeExtn(Xfer->DestFinalName, p_DestName, p_Var, EXTN_APPEND);
    }

    p_Var=GetVar(Cmd->Vars, "SourceFinalExtn");
    if (StrValid(p_Var)) Xfer->SourceFinalName=CopyPathChangeExtn(Xfer->SourceFinalName, p_DestName, p_Var, EXTN_CHANGE);
    else
    {
        p_Var=GetVar(Cmd->Vars, "SourceAppendExtn");
        if (StrValid(p_Var)) Xfer->SourceFinalName=CopyPathChangeExtn(Xfer->SourceFinalName, p_DestName, p_Var, EXTN_APPEND);
    }

    p_Var=GetVar(Cmd->Vars, "PostTransferHookScript");
    if (StrValid(p_Var)) Xfer->PostHookScript=MCopyStr(Xfer->PostHookScript, p_DestName, p_Var, NULL);

    p_Var=GetVar(Cmd->Vars, "PreTransferHookScript");
    if (StrValid(p_Var)) Xfer->PreHookScript=MCopyStr(Xfer->PreHookScript, p_DestName, p_Var, NULL);


    p_Var=GetVar(Cmd->Vars, "DestBackups");
    if (StrValid(p_Var)) Xfer->DestBackups=atoi(p_Var);

    if (! StrValid(Xfer->SourceFinalName)) Xfer->SourceFinalName=CopyStr(Xfer->SourceFinalName, Xfer->Path);

    return(Xfer);
}





int TransferPreProcess(TFileTransfer *Xfer)
{
    struct stat FStat;
    char *Tempstr=NULL, *Path=NULL;
    int i;

    if (FileStoreItemExists(Xfer->ToFS, Xfer->DestFinalName, FTYPE_FILE))
    {
        if (! (Xfer->Flags & (XFER_FLAG_FORCE | XFER_FLAG_RESUME))) return(XFER_DEST_EXISTS);
    }

    if (StrValid(Xfer->PreHookScript))
    {
        if (Xfer->Flags & XFER_FLAG_DOWNLOAD) Tempstr=MCopyStr(Tempstr, Xfer->PreHookScript, " ", Xfer->DestFinalName, NULL);
        else Tempstr=MCopyStr(Tempstr, Xfer->PreHookScript, " ", Xfer->SourceFinalName, NULL);
        system(Tempstr);
    }


    if (Xfer->EncryptType > ENCRYPT_NONE)
    {
        if (! StrValid(Xfer->EncryptKey)) Xfer->EncryptKey=UI_AskPassword(Xfer->EncryptKey);

        if (Xfer->Flags & XFER_FLAG_UPLOAD)
        {
            //as it's important for the user to know that their file is being
            //sent encrypted, we don't use OUTPUT_VERBOSE here
            Xfer->PreProcessedPath=EncryptFile(Xfer->PreProcessedPath, Xfer->Path, Xfer->EncryptKey, Xfer->EncryptType);

            stat(Xfer->PreProcessedPath, &FStat);
            Xfer->Size=FStat.st_size;
            Xfer->TransferName=CatStr(Xfer->TransferName, ".enc");
            Xfer->DestFinalName=CatStr(Xfer->DestFinalName, ".enc");
            UI_Output(0, "Encrypting: %s -> %s -> %s", Xfer->Path, Xfer->PreProcessedPath, Xfer->DestFinalName);
        }
    }

    if (Xfer->DestBackups > 0)
    {
        Tempstr=FormatStr(Tempstr, "%s.%d", Xfer->DestFinalName, Xfer->DestBackups);
        FileStoreUnlinkPath(Xfer->ToFS, Tempstr);
        for (i=Xfer->DestBackups; i > 1; i--)
        {
            Path=FormatStr(Path, "%s.%d", Xfer->DestFinalName, i-1);
            Tempstr=FormatStr(Tempstr, "%s.%d", Xfer->DestFinalName, i);
            UI_Output(0, "Backup: %s -> %s", Path, Tempstr);
            FileStoreRename(Xfer->ToFS, Path, Tempstr);
        }

        Tempstr=FormatStr(Tempstr, "%s.%d", Xfer->DestFinalName, 1);
        UI_Output(0, "Backup: %s -> %s", Xfer->DestFinalName, Tempstr);
        FileStoreRename(Xfer->ToFS, Xfer->DestFinalName, Tempstr);
    }


    Destroy(Tempstr);
    Destroy(Path);

    return(XFER_OKAY);
}



int TransferPostProcess(TFileTransfer *Xfer)
{
    TFileStore *FromFS, *ToFS;
    char *Tempstr=NULL, *Path=NULL;
    ListNode *Node;
    TFileItem *FI;

    FromFS=Xfer->FromFS;
    ToFS=Xfer->ToFS;

    if (Xfer->Flags & XFER_FLAG_DOWNLOAD)
    {
        Path=MCopyStr(Path, ToFS->CurrDir, "/", Xfer->TransferName, NULL);
        fflush(NULL);
        UI_Output(UI_OUTPUT_VERBOSE, "Decrypting: %s", Path);
        Tempstr=DecryptFile(Tempstr, Path, Xfer->EncryptKey, Xfer->EncryptType);
    }
    else
    {
        Path=CopyStr(Path, Xfer->PreProcessedPath);
    }

    //if we were doing either an upload or a download and encryption was used, then
    //we will need to tidy up
    if (Xfer->EncryptType > ENCRYPT_NONE)
    {
        //if we are using encryption, than Xfer->Path will point to a temporary file
        UI_Output(UI_OUTPUT_VERBOSE, "unlink encrypted file: %s", Path);
        unlink(Path);
    }

    if ( StrValid(Xfer->DestFinalName) && (strcmp(Xfer->TransferName, Xfer->DestFinalName) != 0) )
    {
        UI_Output(UI_OUTPUT_VERBOSE, "Renaming Destination: %s %s", Xfer->TransferName, Xfer->DestFinalName);
        FileStoreRename(ToFS, Xfer->TransferName, Xfer->DestFinalName);
    }
    else Xfer->DestFinalName=CopyStr(Xfer->DestFinalName, Xfer->TransferName);

    if ( StrValid(Xfer->SourceFinalName) && (strcmp(Xfer->Path, Xfer->SourceFinalName) != 0) )
    {
        UI_Output(UI_OUTPUT_VERBOSE, "Renaming Source: %s %s", Xfer->Path, Xfer->SourceFinalName);
        FileStoreRename(FromFS, Xfer->Path, Xfer->SourceFinalName);
    }
    else Xfer->SourceFinalName=CopyStr(Xfer->SourceFinalName, Xfer->Path);

    if (StrValid(Xfer->PostHookScript))
    {
        if (Xfer->Flags & XFER_FLAG_DOWNLOAD) Tempstr=MCopyStr(Tempstr, Xfer->PostHookScript, " ", Xfer->DestFinalName, NULL);
        else Tempstr=MCopyStr(Tempstr, Xfer->PostHookScript, " ", Xfer->SourceFinalName, NULL);
        system(Tempstr);
    }

    time(&Now);


    Destroy(Tempstr);
    Destroy(Path);
}



ListNode *TransferFileGlob(TFileStore *FromFS, TFileStore *ToFS, TCommand *Cmd)
{
    ListNode *DirList=NULL, *tmpList, *Curr, *Next;
    char *Item=NULL;
    const char *ptr;
    TFileItem *FI;

    switch (Cmd->Type)
    {
    case CMD_GET:
    case CMD_PUT:
        DirList=FileStoreGlob(FromFS, Cmd->Target);
        break;

    case CMD_MGET:
    case CMD_MPUT:
        DirList=ListCreate();
        ptr=GetToken(Cmd->Target, "\\S", &Item, GETTOKEN_QUOTES);
        while (ptr)
        {
            tmpList=FileStoreGlob(FromFS, Item);
            Curr=ListGetNext(tmpList);
            while (Curr)
            {
                ListAddNamedItem(DirList, Curr->Tag, Curr->Item);
                Curr=ListGetNext(Curr);
            }
            ListDestroy(tmpList, NULL);
            ptr=GetToken(ptr, "\\S", &Item, GETTOKEN_QUOTES);
        }
        break;
    }


    //remove any 'not included' files, so we know how many files we are really
    //transferring
    Curr=ListGetNext(DirList);
    while (Curr)
    {
        Next=ListGetNext(Curr);
        FI=(TFileItem *) Curr->Item;
        if (! FileIncluded(Cmd, FI, FromFS, ToFS))
        {
            UI_Output(UI_OUTPUT_VERBOSE, "TRANSFER NOT NEEDED: %s", FI->path);
            ListDeleteNode(Curr);
            FileItemDestroy(FI);
        }
        Curr=Next;
    }


    Destroy(Item);
    return(DirList);
}



//setup a buffer to hold the transfer data. We want this to be as big as
//we can reasonably make it, while not causing an 'out of data' fault.
//Many transfer types send data in chunks, and if they are http based
//that may mean a reconnection/renegotiation on each chunk. So bigger
//transfer buffers mean faster transfers.
static long TransferSetupBuffer(char **Buffer, TFileStore *FromFS, TFileStore *ToFS)
{
    long len;
    const char *ptr;

//if we're on a 16-bit system (probably not possible, but if)
//then don't try to be clever, just go with 4k
    if (sizeof(long) == 2) len=4096;
//otherwise start at 4 meg
    else len=4 * 1024 * 1024;

   //Does either filestore specify a limit?
   ptr=GetVar(FromFS->Vars, "max_transfer_chunk");
   if (StrValid(ptr) && (atol(ptr) < len)) len=atol(ptr);

   ptr=GetVar(ToFS->Vars, "max_transfer_chunk");
   if (StrValid(ptr) && (atol(ptr) < len)) len=atol(ptr);

    *Buffer = SetStrLen(*Buffer, len);
    while ((Buffer == NULL) & (len > 4096))
    {
        len=len / 1024;
        *Buffer = SetStrLen(*Buffer, len);
    }

    if (! Buffer) return(0);
    return(len);
}



static int TransferCopyFile(TFileTransfer *Xfer, STREAM *FromS, STREAM *ToS)
{
    int len, result;
    char *Tempstr=NULL;
    TFileStore *FromFS, *ToFS;

    FromFS=Xfer->FromFS;
    ToFS=Xfer->ToFS;

    len=TransferSetupBuffer(&Tempstr, FromFS, ToFS);
    if (len==0)
    {
        UI_Output(UI_OUTPUT_ERROR, "Can't allocate memory buffer for file transfers.");
        return(0);
    }

    UI_Output(UI_OUTPUT_DEBUG, "Using %s buffer for file transfers", ToIEC((double) len, 1));
    result=FromFS->ReadBytes(FromFS, FromS, Tempstr, Xfer->Offset, len);
    while (result != STREAM_CLOSED)
    {
        result=ToFS->WriteBytes(ToFS, ToS, Tempstr, Xfer->Offset, result);
        if (result ==STREAM_CLOSED)
        {
            UI_Output(UI_OUTPUT_ERROR, "write failed");
            break;
        }

        Xfer->Offset+=result;

        Xfer->Transferred+=result;
        if ((Settings->Flags & SETTING_PROGRESS) && Xfer->ProgressFunc) Xfer->ProgressFunc(Xfer);
        result=FromFS->ReadBytes(FromFS, FromS, Tempstr, Xfer->Offset, len);
    }

    Destroy(Tempstr);

    return(Xfer->Offset);
}


int TransferFile(TFileTransfer *Xfer)
{
    int RetVal, result;
    char *Tempstr=NULL, *Name=NULL, *SrcPath=NULL;
    char *p_Path;
    TFileStore *FromFS, *ToFS;
    STREAM *FromS, *ToS;

    Xfer->StartTime=GetTime(TIME_CENTISECS);
    result=TransferPreProcess(Xfer);

    if (result != XFER_OKAY) return(result);

    FromFS=Xfer->FromFS;
    ToFS=Xfer->ToFS;

    //if we have pre-processed the file in some way (encryption, compression, etc) and produced an output
    //file for upload, then use the path to that rather than the path to the origin file
    if (StrValid(Xfer->PreProcessedPath)) p_Path=Xfer->PreProcessedPath;
    else p_Path=Xfer->Path;

    Tempstr=FileStoreReformatPath(Tempstr, p_Path, FromFS);
    FromS=FromFS->OpenFile(FromFS, Tempstr, XFER_FLAG_READ | (Xfer->Flags & XFER_FLAGS_OPEN_FILE), Xfer->Offset, 0);

    if (! FromS) RetVal=XFER_SOURCE_FAIL;
    else
    {
        Name=FileStoreReformatPath(Name, GetBasename(Xfer->TransferName), ToFS);
        ToS=ToFS->OpenFile(ToFS, Name, XFER_FLAG_WRITE | (Xfer->Flags & XFER_FLAGS_OPEN_FILE), Xfer->Offset, Xfer->Size);
        if (! ToS)
        {
            RetVal=XFER_DEST_FAIL;
            FromFS->CloseFile(FromFS, FromS);
        }
        else
        {
            TransferCopyFile(Xfer, FromS, ToS);
            FromFS->CloseFile(FromFS, FromS);

            if (! ToFS->CloseFile(ToFS, ToS)) RetVal=XFER_DEST_FAIL;
            else if (Xfer->Offset != Xfer->Size) RetVal=XFER_SHORT_FAIL;
            else
            {
                TransferPostProcess(Xfer);
                RetVal=XFER_OKAY;
            }
        }
    }


    Destroy(Tempstr);
    Destroy(SrcPath);
    Destroy(Name);

    return(RetVal);
}


int TransferFileCommand(TFileStore *FromFS, TFileStore *ToFS, TCommand *Cmd)
{
    TFileTransfer *Xfer;
    TFileItem *FI;
    ListNode *DirList, *Curr;
    int result, transfers=0;
    char *HashType=NULL, *Tempstr=NULL;
    float duration;

    if (! StrValid(Cmd->Target))
    {
        UI_Output(UI_OUTPUT_ERROR, "No target path");
        return(FALSE);
    }

    DirList=TransferFileGlob(FromFS, ToFS, Cmd);
    Curr=ListGetNext(DirList);
    if (! Curr) UI_Output(UI_OUTPUT_ERROR, "No files matching '%s'", Cmd->Target);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        Xfer=FileTransferFromCommand(Cmd, FromFS, ToFS, FI);
        Xfer->ProgressFunc=UI_TransferProgress;
        Xfer->TotalFiles=ListSize(DirList);
        Xfer->CurrFileNum=transfers;

        CommandActivateTimeout();
        result=TransferFile(Xfer);
        CommandDeactivateTimeout();

        switch (result)
        {
        case XFER_SOURCE_FAIL:
            HandleEvent(FromFS, UI_OUTPUT_ERROR, "Failed to open source file '$(path)'.", Xfer->Path, "");
            break;

        case XFER_DEST_FAIL:
            HandleEvent(ToFS, UI_OUTPUT_ERROR, "Failed to open destination file '$(path)'.", GetBasename(Xfer->Path), "");
            break;

        case XFER_DEST_EXISTS:
            HandleEvent(ToFS, UI_OUTPUT_ERROR, "Destination '$(path)' already exists. Transfer aborted.", GetBasename(Xfer->Path), "");
            break;

        case XFER_SHORT_FAIL:
            HandleEvent(ToFS, UI_OUTPUT_ERROR, "Transfer failed: transfer of $(path) ended early", Xfer->Path,  "");
            break;

        default:
            duration=((float) (GetTime(TIME_CENTISECS) - Xfer->StartTime)) / 100.0;
            UI_Output(UI_OUTPUT_SUCCESS, "TRANSFERRED: %llu/%llu %s (%sb) in %0.2fsecs ~>", Xfer->CurrFileNum+1, Xfer->TotalFiles, FI->name, ToMetric(Xfer->Transferred, 1), duration);
            if (Xfer->Flags & XFER_FLAG_DOWNLOAD) HandleEvent(FromFS, 0, "$(filestore): Transferred: $(path)", Xfer->Path, "");
            else HandleEvent(ToFS, 0, "$(filestore): Uploaded: $(path)", Xfer->Path, "");

            if (Cmd->Flags & CMD_FLAG_INTEGRITY)
            {

                switch(FileStoreCompareFiles(FromFS, ToFS, Xfer->Path, Xfer->DestFinalName, &HashType))
                {
                case CMP_MATCH:
                case CMP_LOCAL_NEWER:
                case CMP_REMOTE_NEWER:
                    Tempstr=MCopyStr(Tempstr,	"$(filestore): Transfer integrity ~gconfirmed~0 by ", HashType, NULL);
                    HandleEvent(ToFS, 0, Tempstr, "", "");
                    break;

                default:
                    HandleEvent(ToFS, 0, "$(filestore): Transfer integrity check ~rFAILED~0", "", "");
                    break;
                }
            }
            break;
        }


        UI_Output(0, "");

        FileStoreDirListUpdateItem(ToFS, FTYPE_FILE, Xfer->DestFinalName, Xfer->Offset, 0);
        FileTransferDestroy(Xfer);
        transfers++;
        if ((Cmd->NoOfItems > 0) && (transfers >= Cmd->NoOfItems)) break;

        Curr=ListGetNext(Curr);
    }

    FileStoreDirListFree(FromFS, DirList);

    Destroy(HashType);
    Destroy(Tempstr);

    return(TRUE);
}

