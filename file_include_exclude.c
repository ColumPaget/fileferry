#include "file_include_exclude.h"

int FileIncluded(TCommand *Cmd, TFileItem *FI, TFileStore *FS, TFileStore *ToFS)
{
    TFileItem *Existing;
    int RetVal=TRUE;
    const char *ptr, *p_ID;
    char *Tempstr=NULL;
    time_t When;

    if (strcmp(FI->name, ".")==0) return(FALSE);
    if (strcmp(FI->name, "..")==0) return(FALSE);
    if (FS->Flags & FILESTORE_NOPATH) p_ID=FI->name;
    else p_ID=FI->path;


    if (Cmd->Flags & CMD_FLAG_SYNC)
    {
        Existing=ToFS->FileInfo(ToFS, p_ID);
        if (Existing) return(FALSE);
    }

    ptr=GetVar(Cmd->Vars, "Time:Newer");
    if (StrValid(ptr))
    {
        Tempstr=DatePartialToFull(Tempstr, ptr);
        When=DateStrToSecs("%Y/%m/%d %H:%M:%S", Tempstr, NULL);
        if (FI->mtime > When) RetVal=TRUE;
        else RetVal=FALSE;
    }

    ptr=GetVar(Cmd->Vars, "Time:Older");
    if (StrValid(ptr))
    {
        Tempstr=DatePartialToFull(Tempstr, ptr);
        When=DateStrToSecs("%Y/%m/%d %H:%M:%S", Tempstr, NULL);
        if (FI->mtime < When) RetVal=TRUE;
        else RetVal=FALSE;
    }

    if ( (Cmd->Flags & CMD_FLAG_FILES_ONLY) && (FI->type != FTYPE_FILE) ) RetVal=FALSE;
    if ( (Cmd->Flags & CMD_FLAG_DIRS_ONLY) && (FI->type != FTYPE_DIR) ) RetVal=FALSE;

    if ( StrValid(Cmd->Excludes) && FileInPatternList(p_ID, Cmd->Excludes) ) RetVal=FALSE;
    if ( StrValid(Cmd->Includes) && FileInPatternList(p_ID, Cmd->Includes) ) RetVal=TRUE;

    Destroy(Tempstr);
    return(RetVal);
}

