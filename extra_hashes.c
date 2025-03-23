#include "extra_hashes.h"


/*
Some filestores implement weird hashing formats. 
Really these should probably be included in the filestore driver
but it's simpler to implement them outside of the driver
to prevent having to lookup the driver when using them
to compare a hash against the local disk, as the local disk driver
doesn't want to be calling another driver
*/

char *DropBoxHashFile(char *RetStr, STREAM *S)
{
    char *Buffer=NULL;
    char *RawHash=NULL;
    char *HashChunk=NULL;
    int result, hlen, rlen=0;
#define CHUNK_SIZE (4 * 1024 * 1024)

    if (! S) return(NULL);

    Buffer=SetStrLen(Buffer, CHUNK_SIZE);

    result=STREAMReadBytes(S, Buffer, CHUNK_SIZE);
    while (result > 0)
    {
        HashChunk=CopyStr(HashChunk, "");
        hlen=HashBytes(&HashChunk, "sha256", Buffer, result, 0);
        RawHash=SetStrLen(RawHash, rlen+hlen);
        memcpy(RawHash + rlen, HashChunk, hlen);
        rlen+=hlen;
        result=STREAMReadBytes(S, Buffer, CHUNK_SIZE);
    }

    HashBytes(&RetStr, "sha256", RawHash, rlen, ENCODE_HEX);


    Destroy(RawHash);
    Destroy(HashChunk);
    Destroy(Buffer);

    return(RetStr);
}
