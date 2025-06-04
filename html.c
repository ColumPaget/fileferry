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
    const char *ptr, *tptr;

    ptr=GetNameValuePair(Args, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if ( (strcasecmp(Name, "src")==0) || (strcasecmp(Name, "href")==0) )
        {
            if (IsDownloadableURL(Value))
            {
                FI=FileItemCreate(Value, 0, 0, 0);
                if (FI)
                {
                    tptr=strrchr(FI->name, '/');
                    if (tptr && (*(tptr+1)=='\0'))
                    {
                        FI->type=FTYPE_DIR;
                        StrTrunc(FI->name, tptr - FI->name);
                    }
                    ListAddNamedItem(FileList, FI->name, FI);
                }
            }
        }

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);
}


int HTML_ListDir(ListNode *FileList, const char *HTML)
{
    char *Namespace=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    int RetVal=FALSE;

    ptr=XMLGetTag(HTML, &Namespace, &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "html")==0) RetVal=TRUE; //it's an html document, maybe an empty one, but still.
        else if (strcasecmp(Name, "a")==0) //it has an anchor tag!
        {
            HTML_ListDir_HTMLTag(FileList, Value);
            RetVal=TRUE;
        }
        ptr=XMLGetTag(ptr, &Namespace, &Name, &Value);
    }

    Destroy(Namespace);
    Destroy(Name);
    Destroy(Value);

    return(RetVal);
}

