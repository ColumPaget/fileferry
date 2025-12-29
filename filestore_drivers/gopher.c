#include "pop3.h"
#include "inet_protocols.h"
#include "../errors_and_logging.h"
#include "../ui.h"
#include "../password.h"



static STREAM *Gopher_OpenFile(TFileStore *FS, const char *Path, int OpenMode, uint64_t Offset, uint64_t Size)
{
    char *Tempstr=NULL, *Verbiage=NULL;
    const char *ptr;

    if (FS->S == NULL) FS->Connect(FS);
    Tempstr=MCopyStr(Tempstr, Path, "\r\n", NULL);
    STREAMWriteLine(Tempstr, FS->S);
    STREAMFlush(FS->S);

    Destroy(Tempstr);
    Destroy(Verbiage);

    return(FS->S);
}

static int Gopher_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t Offset, uint32_t Len)
{
    int len=0;

    len=STREAMReadBytes(S, Buffer, Len);

    return(len);
}


static int Gopher_CloseFile(TFileStore *FS, STREAM *S)
{
    if (FS->S) 
    {
			STREAMClose(FS->S);
      FS->S=NULL;
    }
    return(TRUE);
}




static TFileItem *GopherDecodeItem(const char *Line)
{
    const char *ptr;
    char Type;
    TFileItem *FI;


    ptr=Line;
    Type=*ptr;

    if (Type=='3') return(NULL);
    ptr++;

    if (Type == '1') FI=FileItemCreate("", FTYPE_DIR, 0, 0);
    else FI=FileItemCreate("", FTYPE_FILE, 0, 0);

//Tab not space here
    ptr=GetToken(ptr, "	",  &(FI->name), 0);
    ptr=GetToken(ptr, "	",  &(FI->path), 0);

    return(FI);
}

static ListNode *Gopher_ListDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    ListNode *Files=NULL;
    TFileItem *FI=NULL;
    const char Type, *ptr;

    if (FS->S == NULL) FS->Connect(FS);

    Files=ListCreate();
    Tempstr=MCopyStr(Tempstr, Path, "\r\n", NULL);
    SendLoggedLine(Tempstr, FS, FS->S);

    Tempstr=STREAMReadLine(Tempstr, FS->S);
    while (Tempstr)
    {
        StripTrailingWhitespace(Tempstr);
        FI=GopherDecodeItem(Tempstr);
        if (FI) ListAddNamedItem(Files, FI->name, FI);
        Tempstr=STREAMReadLine(Tempstr, FS->S);
    }

		STREAMClose(FS->S);
    FS->S=NULL;

    Destroy(Tempstr);

    return(Files);
}


static int Gopher_Connect(TFileStore *FS)
{
    char *Proto=NULL, *Host=NULL, *PortStr=NULL, *Path=NULL, *Tempstr=NULL;
    char *ptr;
    int RetVal=FALSE, i;


    ParseURL(FS->URL, &Proto, &Host, &PortStr, NULL, NULL, &Path,  NULL);
    if (! StrValid(PortStr))
    {
        if (strcasecmp(Proto, "gophers")==0) PortStr=CopyStr(PortStr, "995");
        else PortStr=CopyStr(PortStr, "70");
    }

    if (strcasecmp(Proto, "gophers")==0)
    {
        Tempstr=MCopyStr(Tempstr, "tls:", Host, ":", PortStr, NULL);
        FS->Flags |= FILESTORE_TLS;
    }
    else Tempstr=MCopyStr(Tempstr, "tcp:", Host, ":", PortStr, NULL);

    FS->S=STREAMOpen(Tempstr, "");
    if (FS->S)
    {
        if (FS->Flags & FILESTORE_TLS) FileStoreRecordCipherDetails(FS, FS->S);
        RetVal=TRUE;
    }


    Destroy(Proto);
    Destroy(Host);
    Destroy(Path);
    Destroy(PortStr);
    Destroy(Tempstr);

    return(RetVal);

}


int Gopher_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS;
    FS->Connect=Gopher_Connect;
    FS->ListDir=Gopher_ListDir;
    FS->OpenFile=Gopher_OpenFile;
    FS->CloseFile=Gopher_CloseFile;
    FS->ReadBytes=Gopher_ReadBytes;

    return(TRUE);
}
