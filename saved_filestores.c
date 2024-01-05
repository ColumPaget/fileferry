#include "saved_filestores.h"
#include "settings.h"


void SavedFilestoreLoad(const char *Path, ListNode *FSList)
{
    STREAM *S;
    TFileStore *FS;
    char *Tempstr=NULL;

    S=STREAMOpen(Path, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            FS=FileStoreParse(Tempstr);
            if (FS) ListAddNamedItem(FSList, FS->Name, FS);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }
    Destroy(Tempstr);
}


ListNode *SavedFilestoresLoad(const char *PathList)
{
    char *Path=NULL;
    ListNode *FSList=NULL;
    const char *ptr;

    FSList=ListCreate();

    ptr=GetToken(PathList, ":", &Path, 0);
    while (ptr)
    {
        SavedFilestoreLoad(Path, FSList);
        ptr=GetToken(ptr, ":", &Path, 0);
    }

    Destroy(Path);

    return(FSList);
}


STREAM *SavedFilesOpenSave(const char *PathList)
{
    STREAM *S=NULL;
    char *Path=NULL;
    const char *ptr;

    ptr=GetToken(PathList, ":", &Path, 0);
    while (ptr)
    {
        MakeDirPath(Path, 0700);
        S=STREAMOpen(Path, "w");
        if (S) break;
        ptr=GetToken(ptr, ":", &Path, 0);
    }

    Destroy(Path);
    return(S);
}


int SavedFileStoresSave(const char *PathList, ListNode *FileStores)
{
    char *Tempstr=NULL;
    STREAM *S;
    TFileStore *FS;
    ListNode *Curr;

    S=SavedFilesOpenSave(PathList);
    if (S)
    {
        Curr=ListGetNext(FileStores);
        while (Curr)
        {
            FS=(TFileStore *) Curr->Item;
            Tempstr=MCopyStr(Tempstr, FS->Name, " ", FS->URL, FS->CurrDir, NULL);
            if (StrValid(FS->User)) Tempstr=MCatStr(Tempstr, " user='", FS->User,"'",  NULL);
            if (StrValid(FS->Pass)) Tempstr=MCatStr(Tempstr, " password='", FS->Pass,"'",  NULL);
            if (StrValid(FS->Encrypt)) Tempstr=MCatStr(Tempstr, " encrypt='", FS->Encrypt,"'",  NULL);
            if (StrValid(FS->CredsFile)) Tempstr=MCatStr(Tempstr, " credsfile='", FS->Encrypt, "'", NULL);
            if (StrValid(FS->Proxy)) Tempstr=MCatStr(Tempstr, " proxy='", FS->Proxy, "'", NULL);
            Tempstr=CatStr(Tempstr, "\n");
            STREAMWriteLine(Tempstr, S);
            Curr=ListGetNext(Curr);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);

    return(TRUE);
}



int SavedFileStoresAdd(TFileStore *FS)
{
    ListNode *List, *Curr;

    List=SavedFilestoresLoad(Settings->FileStoresPath);
    Curr=ListFindNamedItem(List, FS->Name);
    if (Curr)
    {
        FileStoreDestroy((TFileStore *) Curr->Item);
        ListDeleteNode(Curr);
    }

    ListAddNamedItem(List, FS->Name, FS);
    SavedFileStoresSave(Settings->FileStoresPath, List);

    ListDestroy(List, FileStoreDestroy);

    return(TRUE);
}



TFileStore *SavedFileStoresFind(const char *Name)
{
    ListNode *List, *Node;
    TFileStore *FS=NULL;

    List=SavedFilestoresLoad(Settings->FileStoresPath);
    Node=ListFindNamedItem(List, Name);
    if (Node)
    {
        FS=(TFileStore *) Node->Item;
        Node->Item=NULL;
    }

    ListDestroy(List, FileStoreDestroy);

    return(FS);
}


TFileStore *SavedFileStoresList()
{
    ListNode *List, *Node;
    TFileStore *FS=NULL;

    List=SavedFilestoresLoad(Settings->FileStoresPath);
    Node=ListGetNext(List);
    while (Node)
    {
        FS=(TFileStore *) Node->Item;
        printf("%-20s  %s\n", FS->Name, FS->URL);
        Node=ListGetNext(Node);
    }

    ListDestroy(List, FileStoreDestroy);

    return(FS);
}
