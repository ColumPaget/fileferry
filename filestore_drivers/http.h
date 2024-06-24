#ifndef FILEFERRY_HTTP_H
#define FILEFERRY_HTTP_H

#include "../filestore.h"


#define HTTP_OKAY 0
#define HTTP_AUTH_FAIL 1
#define HTTP_ERROR 2
#define HTTP_NOT_SUPPORTED 4


STREAM *HTTP_OpenURL(TFileStore *FS, const char *Method, const char *URL, const char *ExtraArgs, const char *ContentType, int ContentSize, int ContentOffset, int DavDepth);
int HTTP_CheckResponseCode(STREAM *S);
int HTTP_CheckResponse(TFileStore *FS, STREAM *S);
int HTTP_BasicCommand(TFileStore *FS, const char *Target, const char *Method, const char *ExtraArgs);
char *HTTP_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName);
int HTTP_Attach(TFileStore *FS);

#endif
