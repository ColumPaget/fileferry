#include "filestore_dirlist.h"
#include "commands.h"
#include <fnmatch.h>

void FileStoreDirListFree(TFileStore *FS, ListNode *Dir)
{
    if (FS->DirList==Dir) return;

    ListDestroy(Dir, FileItemDestroy);
}



void FileStoreDirListAddItem(TFileStore *FS, int Type, const char *Name, uint64_t Size)
{
    TFileItem *FI;
    char *Path=NULL;

    if (! FS->DirList) FS->DirList=ListCreate();
    Path=FileStoreReformatPath(Path, Name, FS);
    FI=FileItemCreate(Path, Type, Size, 0);
    FI->mtime=Now;
    ListAddNamedItem(FS->DirList, FI->name, FI);

    Destroy(Path);
}


void FileStoreDirListRemoveItem(TFileStore *FS, const char *Path)
{
    TFileItem *FI;
    ListNode *Curr;

    Curr=ListFindNamedItem(FS->DirList, GetBasename(Path));
    if (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        FI->type=FTYPE_DELETED;
    }
}


ListNode *FileStoreDirListMatch(TFileStore *FS, ListNode *InputList, const char *Match)
{
    ListNode *GlobList, *Curr;
    TFileItem *Item;
    const char *rname;

    GlobList=ListCreate();
    if (InputList==NULL) InputList=FS->DirList;
    Curr=ListGetNext(InputList);
    while (Curr)
    {
        rname=GetBasename(Curr->Tag);
        if ((! StrValid(Match)) || (fnmatch(Match, rname, 0)==0))
        {
            Item=FileItemClone((TFileItem *) Curr->Item);
            if (Item->type != FTYPE_DELETED) ListAddNamedItem(GlobList, Item->name, Item);
        }

        Curr=ListGetNext(Curr);
    }
    return(GlobList);
}



unsigned long FileStoreGetDirListRecurse(TFileStore *FS, ListNode *ReturnList, const char *Above, const char *Level)
{
    char *Tempstr=NULL, *Path=NULL, *Token=NULL;
    ListNode *DirList=NULL, *Curr=NULL;
    TFileItem *FI, *NewFI;
    const char *ptr;

    Tempstr=CopyStr(Tempstr, Above);
    Tempstr=SlashTerminateDirectoryPath(Tempstr);
    Tempstr=CatStr(Tempstr, Level);


    DirList=FS->ListDir(FS, Above);

    ptr=GetToken(Level, "/", &Token, 0);
    Tempstr=MCatStr(Tempstr, "/", Token, NULL);

    Curr=ListGetNext(DirList);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;

        if (fnmatch(Token, FI->name, 0)==0)
        {
            if (FI->type==FTYPE_DIR)
            {
                Tempstr=MCopyStr(Tempstr, Above, "/", FI->name, 0);
                FileStoreGetDirListRecurse(FS, ReturnList, Tempstr, ptr);
            }
            else
            {
                NewFI=FileItemClone(FI);
                NewFI->path=MCopyStr(NewFI->path, Above, "/", FI->name, NULL);
                ListAddNamedItem(ReturnList, FI->name, NewFI);
            }
        }

        Curr=ListGetNext(Curr);
    }

    ListDestroy(DirList, FileItemDestroy);

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Path);


    return(ListSize(ReturnList));
}


ListNode *FileStoreGetDirList(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL, *Above=NULL;
    ListNode *DirList=NULL, *ReturnList=NULL;
    char *ptr;
    int len;

    Tempstr=FileStoreReformatPath(Tempstr, Path, FS);
    len=StrLen(FS->CurrDir);
    if ((strchr(Path, '/')) && (strncasecmp(Tempstr, FS->CurrDir, len)==0))
    {
        ptr=Tempstr+len;
        while (*ptr=='/') ptr++;
        Above=CopyStrLen(Above, Tempstr, len);
        DirList=ListCreate();
        FileStoreGetDirListRecurse(FS, DirList, Above, ptr);
    }
    else DirList=FS->ListDir(FS, Tempstr);

    Destroy(Tempstr);
    Destroy(Above);

    return(DirList);
}




void FileStoreDirListClear(TFileStore *FS)
{
    if (FS->DirList) ListDestroy(FS->DirList, FileItemDestroy);
    FS->DirList=NULL;
}


ListNode *FileStoreDirListRefresh(TFileStore *FS, int Flags)
{

    //if we already have a dir listing, use that
    if ( (! Flags) && (ListSize(FS->DirList) >0) ) return(FS->DirList);

    //clear out our exising dir listing
    FileStoreDirListClear(FS);

    //if force is not set, and we are set to not list directories by default
    //(this is done when working with filestores full of files where we don't care
    //about the items already present (maybe we are just uploading more items)
    //in order to speed up processing
    if (! (Flags & DIR_FORCE))
    {
        if (FS->Flags & FILESTORE_NO_DIR_LIST) return(NULL);
        if (Settings->Flags & SETTING_NO_DIR_LIST) return(NULL);
    }

    //finally, if all of the above to not apply, load a new DirList
    FS->DirList=FileStoreGetDirList(FS, "");

    return(FS->DirList);
}



ListNode *FileStoreGlob(TFileStore *FS, const char *Path)
{
    ListNode *GlobList=NULL, *SrcDir, *Curr;
    char *Tempstr=NULL;
    const char *Match="";


    if (StrValid(Path))
    {
        //absolute path, not something in curr dir
        if (*Path=='/') SrcDir=FileStoreGetDirList(FS, Path);
        else
        {
            if (strchr(Path, '/')) SrcDir=FileStoreGetDirList(FS, Path);
            else if ( (ListSize(FS->DirList)==0) || (strcmp(Path, ".")==0) ) SrcDir=FileStoreDirListRefresh(FS, DIR_FORCE);
            else SrcDir=FS->DirList;
        }

        //use neither basename nor 'GetBasename' as these both exhibit unexpected behavior
	//what we want is a match string to limit searches in SrcDir. If Path ends in '/'
        //then we want this match string to be "" (empty) to indicate 'sort everything'

        //GetBasename will return '/tmp' for '/tmp/', which is normally desirable, but not here, as we already have 'SrcDir'
 
        //basename exists in two versions:
        //1) POSIX basename modifies the contents of Path, and can return '.' if Path is empty or null. It returns '/' if Path is just '/'
        //2) GNU basename doesn't modify path, and returns the empty string when Path ends in '/' or is just '/'

	//props to barracuda156@github.com for pointing me in the direction of this whole mess

        Match=strrchr(Path, '/');
        if (Match) Match++;
        else Match=Path;
    }
    else
    {
        SrcDir=FileStoreDirListRefresh(FS, 0);
        Match="*";
    }

    if (StrValid(Match)) GlobList=FileStoreDirListMatch(FS, SrcDir, Match);
    FileStoreDirListFree(FS, SrcDir);

    Destroy(Tempstr);

    return(GlobList);
}


ListNode *FileStoreReloadAndGlob(TFileStore *FS, const char *Glob)
{
    FileStoreDirListRefresh(FS, DIR_FORCE);
    return(FileStoreGlob(FS, Glob));
}


int FileStoreGlobCount(TFileStore *FS, const char *Path)
{
    ListNode *Items;
    int count;

    Items=FileStoreGlob(FS, Path);
    count=ListSize(Items);
    FileStoreDirListFree(FS, Items);

    return(count);
}



int FileStoreItemExists(TFileStore *FS, const char *FName, int Flags)
{
    ListNode *Curr;
    TFileItem *FI;

    Curr=ListGetNext(FS->DirList);
    while (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        if ( (FI->type != FTYPE_DELETED) && (strcmp(FI->name, FName)==0) )
        {
            if ((Flags & CMD_FLAG_FILES_ONLY) && (FI->type==FTYPE_FILE)) return(TRUE);
            else if ((Flags & CMD_FLAG_DIRS_ONLY) && (FI->type==FTYPE_DIR)) return(TRUE);
            else return(TRUE);
        }
        Curr=ListGetNext(Curr);
    }

    return(FALSE);
}

