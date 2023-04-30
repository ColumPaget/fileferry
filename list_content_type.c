#include "list_content_type.h"
#include "rss.h"
#include "html.h"

int FileListForContentType(ListNode *FileList, const char *Document, const char *ContentType)
{
    char *Token=NULL;

    if (FileList)
    {
        GetToken(ContentType, ";", &Token, 0);
        if (StrValid(Token))
        {
            if (strcmp(Token, "application/rss+xml")==0) RSS_ListDir(FileList, Document);
            else if (strcmp(Token, "application/xml")==0) RSS_ListDir(FileList, Document);
            else if (strcmp(Token, "text/xml")==0) RSS_ListDir(FileList, Document);
            else HTML_ListDir(FileList, Document);
        }
        else HTML_ListDir(FileList, Document);
    }

    Destroy(Token);

    return(TRUE);
}
