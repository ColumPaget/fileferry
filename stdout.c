#include "stdout.h"

static STREAM *ShowFile_OpenSTDOUT(TFileStore *FS, const char *Path, int OpenType, uint64_t Offset, uint64_t Size)
{
    STREAM *S;

    S=STREAMFromFD(1);

    return(S);
}

static int ShowFile_WriteSTDOUT(TFileStore *FS, STREAM *S, char *Bytes, uint64_t offset, unsigned int len)
{
    STREAMWriteBytes(S, Bytes, len);
    return(len);
}

static int ShowFile_CloseSTDOUT(TFileStore *FS, STREAM *S)
{
//don't close stdout!
    STREAMFlush(S);
    STREAMDestroy(S);
    return(TRUE);
}

TFileStore *STDOUTFilestoreCreate()
{
    TFileStore *FS;

    FS=FileStoreCreate();
    FS->OpenFile=ShowFile_OpenSTDOUT;
    FS->WriteBytes=ShowFile_WriteSTDOUT;
    FS->CloseFile=ShowFile_CloseSTDOUT;

    return(FS);
}

