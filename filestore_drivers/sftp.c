#include "sftp.h"
#include "../fileitem.h"
#include "../ui.h"
#include "../errors_and_logging.h"
#include "../settings.h"
#include "../password.h"

#define SFTP_TRY_AGAIN -1
#define SFT_DISCONNECTED -2


#define SSH_FXP_INIT                1
#define SSH_FXP_VERSION             2
#define SSH_FXP_OPEN                3
#define SSH_FXP_CLOSE               4
#define SSH_FXP_READ                5
#define SSH_FXP_WRITE               6
#define SSH_FXP_LSTAT               7
#define SSH_FXP_FSTAT               8
#define SSH_FXP_SETSTAT             9
#define SSH_FXP_FSETSTAT           10
#define SSH_FXP_OPENDIR            11
#define SSH_FXP_READDIR            12
#define SSH_FXP_REMOVE             13
#define SSH_FXP_MKDIR              14
#define SSH_FXP_RMDIR              15
#define SSH_FXP_REALPATH           16
#define SSH_FXP_STAT               17
#define SSH_FXP_RENAME             18
#define SSH_FXP_READLINK           19
#define SSH_FXP_SYMLINK            20
#define SSH_FXP_STATUS            101
#define SSH_FXP_HANDLE            102
#define SSH_FXP_DATA              103
#define SSH_FXP_NAME              104
#define SSH_FXP_ATTRS             105
#define SSH_FXP_EXTENDED          200
#define SSH_FXP_EXTENDED_REPLY    201


#define SSH_FXF_READ            0x00000001
#define SSH_FXF_WRITE           0x00000002
#define SSH_FXF_APPEND          0x00000004
#define SSH_FXF_CREATE          0x00000008
#define SSH_FXF_TRUNC           0x00000010
#define SSH_FXF_EXCL            0x00000020


#define SSH_PASSWORD_REQUESTED              -1
#define SSH_FX_OK                            0
#define SSH_FX_EOF                           1
#define SSH_FX_NO_SUCH_FILE                  2
#define SSH_FX_PERMISSION_DENIED             3
#define SSH_FX_FAILURE                       4
#define SSH_FX_BAD_MESSAGE                   5
#define SSH_FX_NO_CONNECTION                 6
#define SSH_FX_CONNECTION_LOST               7
#define SSH_FX_OP_UNSUPPORTED                8
#define SSH_FX_TIMEOUT                    9999

#ifndef htonll
#if __BIG_ENDIAN__
# define htonll(x) (x)
#else
# define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif
#endif

#ifndef ntohll
#if __BIG_ENDIAN__
# define ntohll(x) (x)
#else
# define ntohll(x) (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif
#endif



typedef struct
{
    uint8_t type;
    uint32_t id;
    int flags;
    char *Handle;
    uint32_t datalen;
    char *data;
    ListNode *Values;
} TPacket;


int pkt_id=0;

static TPacket *SFTP_PacketCreate()
{
    TPacket *Packet;

    Packet=(TPacket *) calloc(1, sizeof(TPacket));
    Packet->Values=ListCreate();
    return(Packet);
}

static void SFTP_PacketDestroy(TPacket *Packet)
{
    Destroy(Packet->Handle);
    Destroy(Packet->data);
    ListDestroy(Packet->Values, Destroy);
    free(Packet);
}


static int SFTP_SendUint32(STREAM *S, uint32_t val)
{
    uint32_t nval;

    nval=htonl(val);
    STREAMWriteBytes(S, (char *) &nval, sizeof(uint32_t));

    return(TRUE);
}


static int SFTP_SendUint64(STREAM *S, uint64_t val)
{
    uint64_t nval;

    nval=htonll(val);
    STREAMWriteBytes(S, (char *) &nval, sizeof(uint64_t));

    return(TRUE);
}



static int SFTP_SendPacketHeader(STREAM *S, int type, int len)
{
    int val;
    char typechar;

    val=htonl(len+1); //+1 for 'type'
    STREAMWriteBytes(S, (char *) &val, sizeof(uint32_t));
    typechar=type & 0xFF;
    STREAMWriteBytes(S, &typechar, 1);

    //we flush because some servers expect the header to come in a seperate packet
    STREAMFlush(S);

    return(TRUE);
}


static int SFTP_SendString(STREAM *S, const char *Str)
{
    int len, val;

    len=StrLen(Str);
    val=htonl(len);
    STREAMWriteBytes(S, (char *) &val, sizeof(uint32_t));
    STREAMWriteBytes(S, Str, len);

    return(TRUE);
}




static const char *SFTP_ExtractString(const char *Buffer, int *len, char **Str)
{
    const char *ptr;

    ptr=Buffer;
    *len=ntohl(* (uint32_t *) ptr);
    ptr+=sizeof(uint32_t);
    *Str=SetStrLen(*Str, *len);
    memcpy(*Str, ptr, *len);
    StrTrunc(*Str, *len);
    ptr+=*len;

    return(ptr);
}


#define SSH_FILEXFER_ATTR_SIZE          0x00000001
#define SSH_FILEXFER_ATTR_UIDGID        0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS   0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME     0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED      0x80000000


static const char *SFTP_ExtractFileAttr(const char *Input, TFileItem *FI)
{
    const char *ptr;
    uint32_t flags, len, count;
    char *Name=NULL, *Tempstr=NULL;
    int i;

    ptr=Input;
    flags=htonl(* (uint32_t *) ptr);
    ptr+=sizeof(uint32_t);

    if (flags & SSH_FILEXFER_ATTR_SIZE)
    {
        FI->size=htonll(* (uint64_t *) ptr);
        ptr+=sizeof(uint64_t);
    }

    if (flags & SSH_FILEXFER_ATTR_UIDGID)
    {
        FI->uid=ntohl(* (uint32_t *) ptr);
        ptr+=sizeof(uint32_t);
        FI->gid=ntohl(* (uint32_t *) ptr);
        ptr+=sizeof(uint32_t);
    }

    if (flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
        FI->perms=ntohl(* (uint32_t *) ptr);
        ptr+=sizeof(uint32_t);
        FileItemGuessType(FI);
    }

    if (flags & SSH_FILEXFER_ATTR_ACMODTIME)
    {
        FI->atime=ntohl(* (uint32_t *) ptr);
        ptr+=sizeof(uint32_t);

        FI->mtime=ntohl(* (uint32_t *) ptr);
        ptr+=sizeof(uint32_t);
    }

    if (flags & SSH_FILEXFER_ATTR_EXTENDED)
    {
        count=ntohl(* (uint32_t *) ptr);
        ptr+=sizeof(uint32_t);

        for (i=0; i < count; i++)
        {
            ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
            ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
        }
    }

    Destroy(Tempstr);
    Destroy(Name);

    return(ptr);
}



static const char *DecodeStatus(int Status)
{
    switch (Status)
    {
    case SSH_FX_OK:
        return("Success");
        break;
    case SSH_FX_EOF:
        return("End Of File");
        break;
    case SSH_FX_NO_SUCH_FILE:
        return("No Such File");
        break;
    case SSH_FX_PERMISSION_DENIED:
        return("Permission Denied");
        break;
    case SSH_FX_FAILURE:
        return("Failure");
        break;
    case SSH_FX_BAD_MESSAGE:
        return("Bad Message");
        break;
    case SSH_FX_NO_CONNECTION:
        return("No Connection");
        break;
    case SSH_FX_CONNECTION_LOST:
        return("Connection Lost");
        break;
    case SSH_FX_OP_UNSUPPORTED:
        return("Unsupported Operation");
        break;
    case SSH_FX_TIMEOUT:
        return("Timed out");
        break;
    }

    return("Unknown Status");
}

static int SFTP_ReadPacket(TFileStore *FS, STREAM *S, TPacket *Packet)
{
    uint32_t len, valint;
    long count=0, code, i, result;
    char *Buffer=NULL, *Tempstr=NULL, *Handle=NULL, *Name=NULL;
    const char *ptr;
    int RetVal=SSH_FX_FAILURE;

    result=STREAMReadBytes(S, (char *) &valint, sizeof(uint32_t));
    if (result==STREAM_TIMEOUT) return(SSH_FX_TIMEOUT);

    ptr=(const char *) &valint;
    if (strncasecmp((const char *) &valint, "pass", 4)==0)
    {
        Tempstr=SetStrLen(Tempstr, 255);
        memset(Tempstr, 0, 255);
        STREAMPeekBytes(S, Tempstr, 10);
        StripTrailingWhitespace(Tempstr);
        if (strcasecmp(Tempstr, "word:")==0)
        {
            STREAMReadBytes(S, Tempstr, 10);
            Destroy(Tempstr);
            return(SSH_PASSWORD_REQUESTED);
        }
    }

    len=ntohl(valint)-1;

    Buffer=SetStrLen(Buffer, len);
    Packet->type=STREAMReadChar(S);
    while (count < len)
    {
        valint=STREAMReadBytes(S, Buffer+count, len-count);
        count+=valint;
    }



    switch (Packet->type)
    {
    case SSH_FXP_INIT:
    case SSH_FXP_VERSION:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        ptr+=sizeof(uint32_t);
        Tempstr=FormatStr(Tempstr, "%d", htonl(valint));
        SetVar(FS->Vars, "ProtocolVersion", Tempstr);
        while (ptr)
        {
            ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
            if (len < 1) break;
            FS->Features=MCatStr(FS->Features,  Tempstr, " ", NULL);
            ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
        }

        RetVal=SSH_FX_OK;
        break;

    case SSH_FXP_STATUS:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        Packet->id=ntohl(valint);
        ptr+=sizeof(uint32_t);

        valint=*(uint32_t *) ptr;
        RetVal=ntohl(valint);
        ptr+=sizeof(uint32_t);

        ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
        SetVar(Packet->Values, "description", Tempstr);
        UI_Output(UI_OUTPUT_DEBUG, "%d STATUS: %d (%s)", Packet->id, RetVal, Tempstr);
        Tempstr=FormatStr(Tempstr, "%d", RetVal);
        SetVar(Packet->Values, "status", Tempstr);
        break;

    case SSH_FXP_HANDLE:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        Packet->id=ntohl(valint);
        ptr+=sizeof(uint32_t);
        ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
        Packet->Handle=EncodeBytes(Packet->Handle, Tempstr, len, ENCODE_BASE64);
        RetVal=SSH_FX_OK;
        UI_Output(UI_OUTPUT_DEBUG, "READ HANDLE: %d Type: %d STATUS: %d (%s)", Packet->id, Packet->type, RetVal, Tempstr);
        break;
    }

    Destroy(Tempstr);
    Destroy(Buffer);
    Destroy(Handle);
    Destroy(Name);

    return(RetVal);
}


static int SFTP_ReadToPacketID(TFileStore *FS, STREAM *S, int PktID, TPacket *Packet)
{
    int result;

    result=SFTP_ReadPacket(FS, S, Packet);
    while (result != SSH_FX_FAILURE)
    {
        if (Settings->Flags & SETTING_DEBUG) fprintf(stderr, "ReadToPacketID: %d - %s. PktID: %d Expected: %d\n", result, DecodeStatus(result), Packet->id, PktID);
        if (Packet->id==PktID) break;

        if (result == SSH_FX_TIMEOUT) break;
        result=SFTP_ReadPacket(FS, S, Packet);
    }

    return(result);
}

static int SFTP_ReadDirectoryPacket(STREAM *S, ListNode *FileList)
{
    uint32_t len, valint, type, id;
    long count=0, i;
    char *Buffer=NULL, *Tempstr=NULL;
    const char *ptr;
    TFileItem *FI;
    int RetVal=SFTP_TRY_AGAIN, result;

    len=STREAMReadBytes(S, (char *) &valint, sizeof(uint32_t));

    //no data to read, come back later
    if (len==0) return(SFTP_TRY_AGAIN);

    //something went wrong, likely connection closed
    if (len < 1) return(FALSE);

    len=ntohl(valint)-1;

    Buffer=SetStrLen(Buffer, len);
    type=STREAMReadChar(S);
    while (count < len)
    {
        valint=STREAMReadBytes(S, Buffer+count, len-count);
        if (valint < 1) break;
        count+=valint;
    }

    switch (type)
    {
    case SSH_FXP_STATUS:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        id=ntohl(valint);
        ptr+=sizeof(uint32_t);

        valint=*(uint32_t *) ptr;
        ptr+=sizeof(uint32_t);

        ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
        UI_Output(UI_OUTPUT_DEBUG, "%d %s", ntohl(valint), Tempstr);
        //SetVar(Packet->Values, "description", Tempstr);
        Tempstr=FormatStr(Tempstr, "%d", ntohl(valint));
        //SetVar(Packet->Values, "status", Tempstr);
        RetVal=FALSE;
        break;


    case SSH_FXP_NAME:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        id=ntohl(valint);
        ptr+=sizeof(uint32_t);

        valint=*(uint32_t *) ptr;
        count=ntohl(valint);
        ptr+=sizeof(uint32_t);

        for (i=0; i < count; i++)
        {
            FI=FileItemCreate("", 0, 0, 0);
            ptr=SFTP_ExtractString(ptr, &len, &FI->path);
            FI->name=CopyStr(FI->name, FI->path);
            ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
            ptr=SFTP_ExtractFileAttr(ptr, FI);

            if (
                (strcmp(FI->name, ".") !=0) &&
                (strcmp(FI->name, "..") !=0)
            ) ListAddNamedItem(FileList, FI->name, FI);
            else FileItemDestroy(FI);
        }
        RetVal=TRUE;
        break;

    }

    Destroy(Tempstr);
    Destroy(Buffer);

    return(RetVal);
}



static int SFTP_ReadDataPacket(STREAM *S, TPacket *Packet, char *Data)
{
    uint32_t len=0, valint;
    long count=0, code, result, i;
    char *Buffer=NULL, *Tempstr=NULL, *Handle=NULL, *Name=NULL;
    const char *ptr;

    result=STREAMReadBytes(S, (char *) &valint, sizeof(uint32_t));
    len=ntohl(valint)-1;

    Buffer=SetStrLen(Buffer, len);
    Packet->type=STREAMReadChar(S);
    while (count < len)
    {
        valint=STREAMReadBytes(S, Buffer+count, len-count);
        count+=valint;
    }

    switch (Packet->type)
    {
    case SSH_FXP_STATUS:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        Packet->id=ntohl(valint);
        ptr+=sizeof(uint32_t);

        valint=*(uint32_t *) ptr;
        ptr+=sizeof(uint32_t);

        ptr=SFTP_ExtractString(ptr, &len, &Tempstr);
        SetVar(Packet->Values, "description", Tempstr);
        if (Settings->Flags & SETTING_DEBUG) fprintf(stderr, "SSH_FXP_STATUS: %d %s\n", ntohl(valint), Tempstr);
        Tempstr=FormatStr(Tempstr, "%d", ntohl(valint));
        SetVar(Packet->Values, "status", Tempstr);
        len=STREAM_CLOSED;
        break;


    case SSH_FXP_DATA:
        ptr=Buffer;
        valint=*(uint32_t *) ptr;
        Packet->id=ntohl(valint);
        ptr += sizeof(uint32_t);
        valint=*(uint32_t *) ptr;
        len=ntohl(valint);
        ptr += sizeof(uint32_t);
        memcpy(Data, ptr, len);
        break;
    }

    Destroy(Tempstr);
    Destroy(Buffer);
    Destroy(Handle);
    Destroy(Name);

    return(len);
}




static int SFTP_SendFileRequest(STREAM *S, int Type, const char *File, uint32_t Flags)
{
    uint32_t len, id;

    len=StrLen(File) + sizeof(uint32_t) +sizeof(uint32_t);
    if (Type == SSH_FXP_OPEN) len+=(2 * sizeof(uint32_t));
    else if (Type == SSH_FXP_SETSTAT) len+=(2 * sizeof(uint32_t));

    SFTP_SendPacketHeader(S, Type,  len);
    SFTP_SendUint32(S, ++pkt_id);
    SFTP_SendString(S, File);

    UI_Output(UI_OUTPUT_DEBUG, "FILE REQUEST: type=%d pkt_id=%d %s", Type, pkt_id, File);

    switch (Type)
    {
    case SSH_FXP_OPEN:
        SFTP_SendUint32(S, Flags);
        SFTP_SendUint32(S, 0);
        break;

    case SSH_FXP_SETSTAT:
        SFTP_SendUint32(S, SSH_FILEXFER_ATTR_PERMISSIONS);
        SFTP_SendUint32(S, Flags);
        break;
    }

    STREAMFlush(S);

    return(pkt_id);
}



static int SFTP_SendFileHandleRequest(STREAM *S, int Type, const char *Base64, int AdditionalLen)
{
    int handle_len, val;
    char *Handle=NULL;

    handle_len=DecodeBytes(&Handle, Base64, ENCODE_BASE64);
    SFTP_SendPacketHeader(S, Type,  sizeof(uint32_t) + sizeof(uint32_t) + handle_len + AdditionalLen);
    SFTP_SendUint32(S, ++pkt_id);

    //send handle itself
    val=htonl(handle_len);
    STREAMWriteBytes(S, (char *) &val, sizeof(uint32_t));
    STREAMWriteBytes(S, Handle, handle_len);

    STREAMFlush(S);

    Destroy(Handle);
    return(TRUE);
}


static int SFTP_SendGetData(STREAM *S, int Type, const char *Handle)
{
    SFTP_SendFileHandleRequest(S, Type, Handle, 0);
    STREAMFlush(S);

    return(TRUE);
}

static int SFTP_SendGetFileData(STREAM *S, int Type, const char *Handle, uint64_t offset, uint32_t len)
{
    uint32_t val;

    SFTP_SendFileHandleRequest(S, Type, Handle, sizeof(uint64_t) + sizeof(uint32_t));
    SFTP_SendUint64(S, offset);
    SFTP_SendUint32(S, len);

    STREAMFlush(S);
    return(TRUE);
}


static int SFTP_SendFileWrite(STREAM *S, const char *Handle, uint64_t offset, uint32_t len, const char *Bytes)
{
    uint32_t val;

    SFTP_SendFileHandleRequest(S, SSH_FXP_WRITE, Handle, sizeof(uint64_t) + sizeof(uint32_t) + len);
    SFTP_SendUint64(S, offset);
    SFTP_SendUint32(S, len);
    STREAMWriteBytes(S, Bytes, len);

    STREAMFlush(S);
    return(TRUE);
}


static char *SFTP_RealPath(char *RetStr, const char *Target, STREAM *S)
{
    TFileItem *FI;
    ListNode *List, *Curr;
    int result;

    RetStr=CopyStr(RetStr, "");
    SFTP_SendFileRequest(S, SSH_FXP_REALPATH, Target, 0);
    List=ListCreate();
    result=SFTP_ReadDirectoryPacket(S, List);
    while (result==SFTP_TRY_AGAIN)
    {
        usleep(100000);
        result=SFTP_ReadDirectoryPacket(S, List);
    }

    Curr=ListGetNext(List);
    if (Curr)
    {
        FI=(TFileItem *) Curr->Item;
        RetStr=CopyStr(RetStr, FI->name);
    }

    ListDestroy(List, FileItemDestroy);

    return(RetStr);
}


//doesn't really chdir, but checks if a directory exists
static int SFTP_ChDir(TFileStore *FS, const char *Path)
{
    char *Tempstr=NULL;
    int result=FALSE;

    Tempstr=SFTP_RealPath(Tempstr, Path, FS->S);
    if (StrValid(Tempstr)) result=TRUE;

    Destroy(Tempstr);

    return(result);
}

static int SFTP_RmDir_Path(TFileStore *FS, const char *Target)
{
    int RetVal=FALSE;
    TPacket *Packet;

    Packet=SFTP_PacketCreate();
    SFTP_SendFileRequest(FS->S, SSH_FXP_RMDIR, Target, 0);
    UI_Output(UI_OUTPUT_DEBUG, "RMDIR: %d %s", pkt_id, Target);
    if (SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet)==SSH_FX_OK) RetVal=TRUE;
    SFTP_PacketDestroy(Packet);

    return(RetVal);
}
static int SFTP_Unlink_Path(TFileStore *FS, const char *Target)
{
    int RetVal=FALSE;
    TPacket *Packet;

    Packet=SFTP_PacketCreate();
    SFTP_SendFileRequest(FS->S, SSH_FXP_REMOVE, Target, 0);
    UI_Output(UI_OUTPUT_DEBUG, "UNLINK: %d %s", pkt_id, Target);
    if (SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet)==SSH_FX_OK) RetVal=TRUE;
    else RetVal=SFTP_RmDir_Path(FS, Target);
    SFTP_PacketDestroy(Packet);

    return(RetVal);
}


static int SFTP_Rename_Path(TFileStore *FS, const char *Target, const char *Dest)
{
    int RetVal=FALSE;
    TPacket *Packet;
    uint32_t len, id;

    Packet=SFTP_PacketCreate();
    len=sizeof(uint32_t) + sizeof(uint32_t) + StrLen(Target) + sizeof(uint32_t) + StrLen(Dest);

    SFTP_SendPacketHeader(FS->S, SSH_FXP_RENAME,  len);
    SFTP_SendUint32(FS->S, ++pkt_id);
    SFTP_SendString(FS->S, Target);
    SFTP_SendString(FS->S, Dest);
    STREAMFlush(FS->S);

    UI_Output(UI_OUTPUT_DEBUG, "RENAME: %d %s -> %s", pkt_id, Target, Dest);
    if (SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet)==SSH_FX_OK) RetVal=TRUE;

    SFTP_PacketDestroy(Packet);

    return(RetVal);
}

static int SFTP_ChMod(TFileStore *FS, const char *Target, int Mode)
{
    int RetVal=FALSE;
    TPacket *Packet;

    Packet=SFTP_PacketCreate();
    SFTP_SendFileRequest(FS->S, SSH_FXP_SETSTAT, Target, Mode);
    if (SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet)==SSH_FX_OK) RetVal=TRUE;
    SFTP_PacketDestroy(Packet);

    return(RetVal);
}



static int SFTP_MkDir(TFileStore *FS, const char *Path, int Mode)
{
    int RetVal=FALSE;
    TPacket *Packet;
    uint32_t len, id;

    len=sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + StrLen(Path);

    SFTP_SendPacketHeader(FS->S, SSH_FXP_MKDIR,  len);
    SFTP_SendUint32(FS->S, ++pkt_id);
    SFTP_SendString(FS->S, Path);
    SFTP_SendUint32(FS->S, SSH_FILEXFER_ATTR_PERMISSIONS);
    SFTP_SendUint32(FS->S, Mode);
    STREAMFlush(FS->S);

    Packet=SFTP_PacketCreate();
    if (SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet)==SSH_FX_OK) RetVal=TRUE;
    SFTP_PacketDestroy(Packet);

    return(RetVal);
}




static STREAM *SFTP_OpenFile(TFileStore *FS, const char *Path, int OpenFlags, uint64_t Offset, uint64_t Size)
{
    TPacket *Packet;
    uint64_t offset=0;
    int SFTPFlags=0;
    const char *ptr;
    STREAM *S=NULL;

    if (OpenFlags & XFER_FLAG_WRITE) SFTPFlags |= SSH_FXF_WRITE|SSH_FXF_CREATE|SSH_FXF_TRUNC;
    else SFTPFlags |= SSH_FXF_READ;

    //append
    //     SFTPFlags |= SSH_FXF_APPEND|SSH_FXF_CREATE;

//clone DualFD

    SFTP_SendFileRequest(FS->S, SSH_FXP_OPEN, Path, SFTPFlags);
    Packet=SFTP_PacketCreate();
    if (SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet)==SSH_FX_OK)
    {
        S=STREAMFromDualFD(FS->S->in_fd, FS->S->out_fd);
        STREAMSetValue(S, "SFTP:Handle", Packet->Handle);
    }
    SFTP_PacketDestroy(Packet);

    return(S);
}

static int SFTP_CloseHandle(TFileStore *FS, STREAM *S, const char *Handle)
{
    TPacket *Packet;
    int i, result;

    if (! StrValid(Handle)) return(FALSE);

    Packet=SFTP_PacketCreate();

    for (i=0; i < 5; i++)
    {
        SFTP_SendFileHandleRequest(S, SSH_FXP_CLOSE, Handle, 0);
        result=SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet);
        if (result == SSH_FX_OK) break;
        sleep(2);
    }

    SFTP_PacketDestroy(Packet);

    STREAMFlush(S);

    return(TRUE);
}


static int SFTP_CloseFile(TFileStore *FS, STREAM *S)
{
    const char *p_handle;

    p_handle=STREAMGetValue(S, "SFTP:Handle");
    return(SFTP_CloseHandle(FS, S, p_handle));
}

static int SFTP_ReadBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    int result;
    TPacket *Packet;
    const char *p_handle;

    Packet=SFTP_PacketCreate();
    p_handle=STREAMGetValue(S, "SFTP:Handle");
    SFTP_SendGetFileData(S, SSH_FXP_READ, p_handle, offset, len);
    result=SFTP_ReadDataPacket(S, Packet, Buffer);
    SFTP_PacketDestroy(Packet);

    return(result);
}


static int SFTP_WriteBytes(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len)
{
    TPacket *Packet;
    const char *p_handle;
    int result;

    Packet=SFTP_PacketCreate();
    p_handle=STREAMGetValue(S, "SFTP:Handle");

    SFTP_SendFileWrite(S, p_handle, offset, len, Buffer);
    result=SFTP_ReadPacket(FS, S, Packet);

    SFTP_PacketDestroy(Packet);

    if (result==SSH_FX_OK) return(len);
    return(-1);
}


static ListNode *SFTP_ListDir(TFileStore *FS, const char *Path)
{
    TPacket *Packet;
    char *Handle=NULL, *Dir=NULL;
    ListNode *FileList=NULL;
    int fcount, result;

    Dir=CopyStr(Dir, Path);
    Packet=SFTP_PacketCreate();
    SFTP_SendFileRequest(FS->S, SSH_FXP_OPENDIR, Dir, 0);
    result=SFTP_ReadToPacketID(FS, FS->S, pkt_id, Packet);
    if (result == SSH_FX_OK)
    {
        Handle=CopyStr(Handle, Packet->Handle);

        FileList=ListCreate();
        while (1)
        {
            SFTP_SendGetData(FS->S, SSH_FXP_READDIR, Handle);
            result=SFTP_ReadDirectoryPacket(FS->S, FileList);
            while (result==SFTP_TRY_AGAIN)
            {
                usleep(100000);
                result=SFTP_ReadDirectoryPacket(FS->S, FileList);
            }
            if (result != TRUE) break;
        }
    } else UI_Output(0, "ERROR: OpenDir: Dir=%s result=%s %d (%s)", Dir, GetVar(Packet->Values, "status"), result, GetVar(Packet->Values, "description"));
    SFTP_CloseHandle(FS, FS->S, Handle);

    SFTP_PacketDestroy(Packet);
    Destroy(Handle);
    Destroy(Dir);

    return(FileList);
}



static int SFTP_Init(TFileStore *FS, STREAM *S)
{
    uint32_t valint;
    TPacket *Packet;

    STREAMSetTimeout(S, 1000);
    SFTP_SendPacketHeader(S, SSH_FXP_INIT,  sizeof(uint32_t));
    valint=htonl(3); //version number, version 3
    STREAMWriteBytes(S, (char *) &valint, sizeof(uint32_t));
    STREAMFlush(S);
    Packet=SFTP_PacketCreate();
    valint=SFTP_ReadPacket(FS, S, Packet);
    SFTP_PacketDestroy(Packet);

    return(valint);
}

static void SSH_Authenticate(TFileStore *FS, STREAM *S, int Try)
{
    TPacket *Packet;
    char *Tempstr=NULL;
    const char *p_Pass;
    int result=FALSE;
    int len;

    STREAMSetTimeout(S, 100);
    Tempstr=SetStrLen(Tempstr, 1024);
    len=STREAMReadBytes(S, Tempstr, 1024);
    strlwr(Tempstr);
    if (strstr(Tempstr, "password:"))
    {
        result=PasswordGet(FS, Try, &p_Pass);
        if (result > 0)
        {
            STREAMWriteBytes(S, p_Pass, result);
            STREAMWriteLine("\n", S);
            STREAMFlush(S);
        }
    }

    Destroy(Tempstr);
}


static STREAM *SFTP_MakeConnection(TFileStore *FS)
{
    STREAM *S=NULL, *Pty=NULL;
    char *Proto=NULL, *Host=NULL, *PortStr=NULL, *User=NULL, *Passwd=NULL, *Path=NULL;
    char *Tempstr=NULL, *Args=NULL;

    //break url up into it's parts
    ParseURL(FS->URL, &Proto, &Host, &PortStr, &User, &Passwd, &Path,  &Args);

    Tempstr=MCopyStr(Tempstr, "ssh -T ", NULL);

    //if we have a user add it to the url we pass to ssh
    if (StrValid(FS->User)) Tempstr=MCatStr(Tempstr, FS->User, "@", NULL);
    //complete url we pass to ssh by appending host. This will either append to user part or be on it's own if no user given
    Tempstr=CatStr(Tempstr, Host);
    //was a port specified?
    if (StrValid(PortStr)) Tempstr=MCatStr(Tempstr, " -p ", PortStr, NULL);
    //was an ssh key/identity file specified that we use to login in stead of a password
    if (StrValid(FS->CredsFile)) Tempstr=MCatStr(Tempstr, " -i ", FS->CredsFile, NULL);
    //final args '-s sftp' specifies we want to use the sftp service
    Tempstr=CatStr(Tempstr, " -e none -s sftp ");

    //launch the ssh command on a psuedo-terminal. The psuedo-terminal (Pty) purely exists so we can capture
    //password requests from the ssh command
    if (STREAMSpawnCommandAndPty(Tempstr, "", &S, &Pty))
    {
        STREAMSetItem(S, "ssh_pty", Pty);
        STREAMSetTimeout(Pty, 1000);
    }

    Destroy(Tempstr);
    Destroy(Proto);
    Destroy(Host);
    Destroy(PortStr);
    Destroy(User);
    Destroy(Passwd);
    Destroy(Path);
    Destroy(Args);

    return(S);
}


static STREAM *SFTP_Launch(TFileStore *FS)
{
    char *Tempstr=NULL, *Args=NULL;
    STREAM *S, *Pty;
    int i;

    S=SFTP_MakeConnection(FS);
    if (S)
    {
        sleep(2);
        Pty=STREAMGetItem(S, "ssh_pty");
        for (i=0; (i < 5) && STREAMCheckForBytes(Pty); i++)
        {
            SSH_Authenticate(FS, Pty, i);
        }

        if (SFTP_Init(FS, S) != SSH_FX_OK)
        {
            printf("ERROR: Bad init response\n");
            STREAMClose(S);
            S=NULL;
        }
        else FS->CurrDir=SFTP_RealPath(FS->CurrDir, ".", S);
    }
    else HandleEvent(FS, 0, "$(filestore): SFTP connect failed.", FS->URL, "");


    Destroy(Tempstr);
    Destroy(Args);

    return(S);
}


static int SFTP_Connect(TFileStore *FS)
{
    STREAM *S;

    S=SFTP_Launch(FS);
    if (! S) return(FALSE);
    FS->S=S;

    FileStoreTestFeatures(FS);

    return(TRUE);
}


int SFTP_Attach(TFileStore *FS)
{
    FS->Flags |= FILESTORE_FOLDERS;

    FS->Type=FILESTORE_SFTP;
    FS->Connect=SFTP_Connect;
    FS->ListDir=SFTP_ListDir;
    FS->MkDir=SFTP_MkDir;
    FS->ChDir=SFTP_ChDir;
    FS->OpenFile=SFTP_OpenFile;
    FS->CloseFile=SFTP_CloseFile;
    FS->ReadBytes=SFTP_ReadBytes;
    FS->WriteBytes=SFTP_WriteBytes;
    FS->UnlinkPath=SFTP_Unlink_Path;
    FS->RenamePath=SFTP_Rename_Path;
    FS->ChMod=SFTP_ChMod;

    //sftp-server seems to fail for 'write' sizes larger than this
    SetVar(FS->Vars, "max_transfer_chunk", "40960");
    return(TRUE);
}

