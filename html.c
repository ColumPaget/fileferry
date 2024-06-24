#include "html.h"
//#include "http.h"
#include "fileitem.h"


int IsDownloadableURL(const char *URL)
{
    if (*URL=='#') return(FALSE);
    if (strncmp(URL, "javascript:", 11)==0) return(FALSE);
    return(TRUE);
}


static void HTML_ListDir_HTMLTag(ListNode *FileList, const char *Args)
{
    TFileItem *FI;
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    ptr=GetNameValuePair(Args, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if ( (strcasecmp(Name, "src")==0) || (strcasecmp(Name, "href")==0) )
        {
            if (IsDownloadableURL(Value))
            {
                FI=FileItemCreate(FI->path, 0, 0, 0);
                ListAddNamedItem(FileList, FI->name, FI);
            }
        }

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);
}


void HTML_ListDir(ListNode *FileList, const char *HTML)
{
    char *Namespace=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;

    ptr=XMLGetTag(HTML, &Namespace, &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "a")==0) HTML_ListDir_HTMLTag(FileList, Value);
        ptr=XMLGetTag(ptr, &Namespace, &Name, &Value);
    }

    Destroy(Namespace);
    Destroy(Name);
    Destroy(Value);
}

