#include "file_include_exclude.h"


time_t DatePartialToSecs(const char *Date)
{
    char *Tempstr=NULL;
		time_t When;

    Tempstr=DatePartialToFull(Tempstr, Date);
    When=DateStrToSecs("%Y/%m/%d %H:%M:%S", Tempstr, NULL);

		Destroy(Tempstr);

return(When);
}


int FileIncluded(TCommand *Cmd, TFileItem *FI, TFileStore *FS, TFileStore *ToFS)
{
    TFileItem *Existing;
    const char *ptr, *p_ID;
    time_t When;
		int val, RetVal=TRUE;

		//never include these
    if (strcmp(FI->name, ".")==0) return(FALSE);
    if (strcmp(FI->name, "..")==0) return(FALSE);


    if (FS->Flags & FILESTORE_NOPATH) p_ID=FI->name;
    else p_ID=FI->path;


    if (Cmd->Flags & CMD_FLAG_SYNC)
    {
        Existing=ToFS->FileInfo(ToFS, p_ID);
        if (Existing) return(FALSE);
    }

		//use RetVal from here on, so that Includes can override it
    ptr=GetVar(Cmd->Vars, "Time:Newer");
    if (StrValid(ptr))
    {
				When=DatePartialToSecs(ptr);
        if (FI->mtime < When) RetVal=FALSE;
    }

    ptr=GetVar(Cmd->Vars, "Time:Older");
    if (StrValid(ptr))
    {
				When=DatePartialToSecs(ptr);
        if (FI->mtime > When) RetVal=FALSE;
    }

    ptr=GetVar(Cmd->Vars, "Size:Smaller");
    if (StrValid(ptr))
    {
				val=FromIEC(ptr, 10);
        if (FI->size > val) RetVal=FALSE;
    }

    ptr=GetVar(Cmd->Vars, "Size:Larger");
    if (StrValid(ptr))
    {
				val=FromIEC(ptr, 10);
        if (FI->size < val) RetVal=FALSE;
    }

    if ( (Cmd->Flags & CMD_FLAG_FILES_ONLY) && (FI->type != FTYPE_FILE) ) RetVal=FALSE;
    if ( (Cmd->Flags & CMD_FLAG_DIRS_ONLY) && (FI->type != FTYPE_DIR) ) RetVal=FALSE;

    if ( StrValid(Cmd->Excludes) && FileInPatternList(p_ID, Cmd->Excludes) ) RetVal=FALSE;
    if ( StrValid(Cmd->Includes) && FileInPatternList(p_ID, Cmd->Includes) ) RetVal=TRUE;

    return(RetVal);
}

