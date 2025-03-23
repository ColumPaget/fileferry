#include "webdav.h"
#include "http.h"
#include "../fileitem.h"
#include "../ui.h"

#define PROPFIND_XML "<?xml version=\"1.0\" encoding=\"utf-8\" ?> <D:propfind xmlns:D=\"DAV:\" > <D:prop> <D:getcontentlength/> <D:resourcetype/> <D:iscollection/> <D:creationdate/> <D:getlastmodified/> <executable xmlns=\"http://apache.org/dav/props/\"/> <md5-checksum xmlns=\"http://subversion.tigris.org/xmlns/dav/\"/> </D:prop> </D:propfind>"

static void HTTP_Webdav_Normalize_Path(TFileStore *FS, TFileItem *FI, const char *URL)
{
    const char *ptr;
    char *Tempstr=NULL, *Path=NULL;

    if (strncmp(URL, "https:",6)==0)
    {
        ParseURL(URL, NULL, NULL, NULL, NULL, NULL, &Tempstr, NULL);
        ptr=Tempstr;
    }
    else ptr=URL;

    Path=HTTPUnQuote(Path, ptr);
    StripDirectorySlash(Path);
    FI->path=CopyStr(FI->path, FileStorePathRelativeToCurr(FS, Path));
    FI->name=CopyStr(FI->name, GetBasename(FI->path));


    Destroy(Tempstr);
    Destroy(Path);
}


const char *HTTP_Webdav_Parse_Propfind(const char *XML, TFileStore *FS, ListNode *FileList)
{
    char *Namespace=NULL, *Tag=NULL, *Value=NULL;
    const char *ptr;
    TFileItem *FI;

    FI=FileItemCreate("", FTYPE_FILE, 0, 0);
    ptr=XMLGetTag(XML, &Namespace, &Tag, &Value);
    while (ptr)
    {

        if (strcasecmp(Tag, "/response")==0) break;
        else if (strcasecmp(Tag,"iscollection")==0)
        {
            ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            if (strcasecmp(Value, "1")==0) FI->type=FTYPE_DIR;
        }
        else if (strcasecmp(Tag,"resourcetype")==0)
        {
            ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            if (strcasecmp(Tag, "collection")==0) FI->type=FTYPE_DIR;
            if (strcasecmp(Tag, "collection/")==0) FI->type=FTYPE_DIR;
        }
        else if (strcasecmp(Tag,"getcontentlength")==0)
        {
            ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            FI->size=atoi(Value);
        }
        else if (strcasecmp(Tag,"getcontenttype")==0) ptr=XMLGetTag(ptr, &Namespace, &Tag, &FI->content_type);
        else if (strcasecmp(Tag, "href")==0)
        {
            ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            HTTP_Webdav_Normalize_Path(FS, FI, Value);
        }
        else if (strcasecmp(Tag, "creationdate")==0)
        {
            ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            FI->ctime=DateStrToSecs("%Y-%m-%dT%H:%M:%S", Value, NULL);
        }
        else if (strcasecmp(Tag, "getlastmodified")==0)
        {
            ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            FI->mtime=DateStrToSecs("%a, %d %b %Y %H:%M:%S", Value, NULL);
        }

        ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
    }

    if (StrValid(FI->name))
    {
        if (strcmp(FS->CurrDir, FI->path) !=0) ListAddNamedItem(FileList, FI->name, FI);
    }

    Destroy(Namespace);
    Destroy(Value);
    Destroy(Tag);

    return(ptr);
}



int Webdav_ListDir(TFileStore *FS, const char *URL, ListNode *FileList)
{
    char *Tempstr=NULL, *Args=NULL;
    char *Namespace=NULL, *Tag=NULL, *Value=NULL;
    const char *ptr;
    int RetVal=FALSE;
    STREAM *S;


    S=HTTP_OpenURL(FS, "PROPFIND", URL, "", "application/xml", StrLen(PROPFIND_XML), 0, 1);
    if (S)
    {
        STREAMWriteLine(PROPFIND_XML, S);
        UI_Output(UI_OUTPUT_DEBUG, "%s", PROPFIND_XML);
        if (STREAMCommit(S))
        {
            RetVal=HTTP_CheckResponseCode(S);
            Tempstr=STREAMReadDocument(Tempstr, S);

            UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);
            if (RetVal==TRUE)
            {
                ptr=XMLGetTag(Tempstr, &Namespace, &Tag, &Value);
                while (ptr)
                {
                    if (FileList && (strcasecmp(Tag, "response")==0)) ptr=HTTP_Webdav_Parse_Propfind(ptr, FS, FileList);
                    ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
                }
            }
            STREAMClose(S);
        }
    }

    Destroy(Namespace);
    Destroy(Tempstr);
    Destroy(Value);
    Destroy(Tag);
    Destroy(Args);

    return(RetVal);
}



char *WebDav_GetQuota(char *RetStr, TFileStore *FS)
{
    char *Tempstr=NULL;
    char *XML=NULL, *Namespace=NULL, *Tag=NULL, *Value=NULL;
    const char *ptr;
    double avail=-1, used=-1, total=-1;
    int result;
    STREAM *S;

    RetStr=CopyStr(RetStr, "");
    XML=CopyStr(XML,
                "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                "<D:propfind xmlns:D=\"DAV:\" >"
                "<D:prop>"
                "  <D:quota-available-bytes/>"
                "  <D:quota-used-bytes/>"
                "</D:prop>"
                "</D:propfind>");

    //ask for props of root directory
    S=HTTP_OpenURL(FS, "PROPFIND", "", "", "application/xml", StrLen(XML), 0, 0);
    if (S)
    {
        STREAMWriteLine(XML, S);
        UI_Output(UI_OUTPUT_DEBUG, "%s", XML);
        STREAMCommit(S);

        result=HTTP_CheckResponseCode(S);

        Tempstr=STREAMReadDocument(Tempstr, S);

        if (result==TRUE)
        {
            UI_Output(UI_OUTPUT_DEBUG, "%s", Tempstr);

            ptr=XMLGetTag(Tempstr, &Namespace, &Tag, &Value);
            while (ptr)
            {
                if (strcasecmp(Tag, "quota-available-bytes")==0)
                {
                    ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
                    if (StrValid(Value)) avail=strtod(Value, NULL);
                }
                if (strcasecmp(Tag, "quota-used-bytes")==0)
                {
                    ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
                    if (StrValid(Value)) used=strtod(Value, NULL);
                }

                ptr=XMLGetTag(ptr, &Namespace, &Tag, &Value);
            }
        }

        STREAMClose(S);

        total=avail+used;
        if (avail > -1)  RetStr=FormatStr(RetStr, "total=%f avail=%f used=%f", total, avail, used);
    }

    DestroyString(Tempstr);
    DestroyString(Namespace);
    DestroyString(Tag);
    DestroyString(Value);
    DestroyString(XML);

    return(RetStr);
}



char *WebDav_GetValue(char *RetStr, TFileStore *FS, const char *Path, const char *ValName)
{
    RetStr=CopyStr(RetStr, "");

    if (strcasecmp(ValName, "DiskQuota")==0) RetStr=WebDav_GetQuota(RetStr, FS);
    else RetStr=HTTP_GetValue(RetStr, FS, Path, ValName);

    return(RetStr);
}
