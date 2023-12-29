#include "filestore_dirlist.h"
#include "commands.h"

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


ListNode *FileStoreGetDirList(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    ListNode *DirList=NULL;

    Tempstr=FileStoreReformatPath(Tempstr, Path, FS);
    DirList=FS->ListDir(FS, Tempstr);

    Destroy(Tempstr);

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


ListNode *FileStoreGlob(TFileStore *FS, const char *Path)
{
    ListNode *GlobList, *SrcDir, *Curr;
    char *Tempstr=NULL;
    const char *Match;

    //absolute path, not something in curr dir

    if (StrValid(Path))
    {
        if (*Path=='/') SrcDir=FileStoreGetDirList(FS, Path);
        else
        {
            if ( (ListSize(FS->DirList)==0) || strchr(Path, '/') || (strcmp(Path, ".")==0) ) SrcDir=FileStoreDirListRefresh(FS, DIR_FORCE);
            SrcDir=FS->DirList;
        }

        //use basename not 'GetBasename' as these behave differently for a path ending in '/'
        //basename will return blank for a path like '/tmp/', whereas GetBasename will return '/tmp'
        Match=basename(Path);
    }
    else
    {
        SrcDir=FileStoreDirListRefresh(FS, 0);
        Match="*";
    }

		GlobList=FileStoreDirListMatch(FS, SrcDir, Match);
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

