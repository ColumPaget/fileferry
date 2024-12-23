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

    if (FileStoreItemExists(Xfer->ToFS, Xfer->DestFinalName, 0))
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
    FileStoreDirListAddItem(ToFS, FTYPE_FILE, Xfer->DestFinalName, Xfer->Offset);


    Destroy(Tempstr);
    Destroy(Path);
}



ListNode *TransferFileGlob(TFileStore *FS, TCommand *Cmd)
{
    ListNode *DirList=NULL, *tmpList, *Curr;
    char *Item=NULL;
    const char *ptr;

    switch (Cmd->Type)
    {
    case CMD_GET:
    case CMD_PUT:
        DirList=FileStoreGlob(FS, Cmd->Target);
        break;

    case CMD_MGET:
    case CMD_MPUT:
        DirList=ListCreate();
        ptr=GetToken(Cmd->Target, "\\S", &Item, GETTOKEN_QUOTES);
        while (ptr)
        {
            tmpList=FileStoreGlob(FS, Item);
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

    Destroy(Item);
    return(DirList);
}



static int TransferCopyFile(TFileTransfer *Xfer, STREAM *FromS, STREAM *ToS)
{
    int len, result;
    char *Tempstr=NULL;
    TFileStore *FromFS, *ToFS;

    FromFS=Xfer->FromFS;
    ToFS=Xfer->ToFS;

    len=4096 * 10;
    Tempstr=SetStrLen(Tempstr, len);
    result=FromFS->ReadBytes(FromFS, FromS, Tempstr, Xfer->Offset, len);
    while (result != STREAM_CLOSED)
    {
        ToFS->WriteBytes(ToFS, ToS, Tempstr, Xfer->Offset, result);
        Xfer->Offset+=result;
        Xfer->Downloaded+=result;
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

    result=TransferPreProcess(Xfer);
    if (result != XFER_OKAY) return(result);

    FromFS=Xfer->FromFS;
    ToFS=Xfer->ToFS;
    Xfer->StartTime=GetTime(0);

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

    if (! StrValid(Cmd->Target))
    {
        UI_Output(UI_OUTPUT_ERROR, "No target path");
        return(FALSE);
    }

    DirList=TransferFileGlob(FromFS, Cmd);
    Curr=ListGetNext(DirList);
    if (! Curr) UI_Output(UI_OUTPUT_ERROR, "No files matching '%s'", Cmd->Target);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if (FileIncluded(Cmd, FI, FromFS, ToFS))
        {
            Xfer=FileTransferFromCommand(Cmd, FromFS, ToFS, FI);
            Xfer->ProgressFunc=UI_TransferProgress;
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

            default:
                UI_Output(UI_OUTPUT_SUCCESS, "TRANSFERRED: %s (%sb) in %ds                           ", FI->name, ToMetric(Xfer->Downloaded, 1), time(NULL) - Xfer->StartTime);
                if (Xfer->Flags & XFER_FLAG_DOWNLOAD) HandleEvent(FromFS, 0, "$(filestore): Downloaded: $(path)", Xfer->Path, "");
                else HandleEvent(ToFS, 0, "$(filestore): Uploaded: $(path)", Xfer->Path, "");

                if (Cmd->Flags & CMD_FLAG_INTEGRITY)
                {
                    switch(FileStoreCompareFiles(FromFS, ToFS, GetBasename(Xfer->Path), GetBasename(Xfer->Path), &HashType))
                    {
                    case CMP_MATCH:
                    case CMP_LOCAL_NEWER:
                    case CMP_REMOTE_NEWER:
                        Tempstr=MCopyStr(Tempstr,	"$(filestore): Transfer integrity confirmed by ", HashType, NULL);
                        HandleEvent(ToFS, 0, Tempstr, "", "");
                        break;

                    default:
                        HandleEvent(ToFS, 0, "$(filestore): Transfer integrity check FAILED", "", "");
                        break;
                    }
                }
                break;
            }


            UI_Output(0, "");
            FileTransferDestroy(Xfer);
            transfers++;
            if ((Cmd->NoOfItems > 0) && (transfers >= Cmd->NoOfItems)) break;
        }
        else UI_Output(UI_OUTPUT_VERBOSE, "TRANSFER NOT NEEDED: %s", FI->path);

        Curr=ListGetNext(Curr);
    }

    FileStoreDirListFree(FromFS, DirList);

    Destroy(HashType);
    Destroy(Tempstr);

    return(TRUE);
}

