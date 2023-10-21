#ifndef FILEFERRY_HTTP_H
#define FILEFERRY_HTTP_H

#include "filestore.h"


#define HTTP_OKAY 0
#define HTTP_AUTH_FAIL 1
#define HTTP_ERROR 2
#define HTTP_NOT_SUPPORTED 4


int IsDownloadableURL(const char *URL);
//char *HTTPSetConfig(char *RetStr, TFileStore *FS, const char *Method, int Depth, const char *ContentType, int ContentLength);
STREAM *HTTPOpenURL(TFileStore *FS, const char *Method, const char *URL, const char *ExtraArgs, const char *ContentType, int ContentSize, int DavDepth);
int HTTPCheckResponseCode(STREAM *S);
int HTTPBasicCommand(TFileStore *FS, const char *Target, const char *Method, const char *ExtraArgs);

int HTTP_Attach(TFileStore *FS);

#endif
