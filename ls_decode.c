#include "ls_decode.h"

TFileItem *Decode_LS_Output(char *CurrDir, char *LsLine)
{
    TFileItem *FI;
    char *Token=NULL, *DateStr=NULL;
    const char *ptr;

    if (! StrValid(LsLine)) return(NULL);
    FI=FileItemCreate("", FTYPE_FILE, 0, 0);

    ptr=GetToken(LsLine,"\\S",&Token,0);
    if (StrLen(Token)==8) //dos style
    {
        ptr=GetToken(ptr,"\\S",&Token,0);
        ptr=GetToken(ptr,"\\S",&Token,0);
        StripTrailingWhitespace(Token);
        if (strcmp(Token,"<DIR>")==0) FI->type=FTYPE_DIR;
        else
        {
            FI->type=FTYPE_FILE;
            FI->size=atoi(Token);
        }
        FI->path=CopyStr(FI->path,CurrDir);
        FI->path=CatStr(FI->path,ptr);
        FI->name=CopyStr(FI->name,ptr);
    }
    else //unix style
    {
        switch (*Token)
        {
        case 'd':
            FI->type=FTYPE_DIR;
            break;
        case 'l':
            FI->type=FTYPE_LINK;
            break;
        case 'c':
            FI->type=FTYPE_CHARDEV;
            break;
        case 'b':
            FI->type=FTYPE_BLOCKDEV;
            break;
        case 's':
            FI->type=FTYPE_SOCKET;
            break;
        default:
            FI->type=FTYPE_FILE;
            break;
        }
        //FI->Permissions=CopyStr(FI->Permissions,Token);

        ptr=GetToken(ptr,"\\S",&Token,0);
        ptr=GetToken(ptr,"\\S",&FI->user,0);
        ptr=GetToken(ptr,"\\S",&FI->group,0);
        ptr=GetToken(ptr,"\\S",&Token,0);
        StripTrailingWhitespace(Token);
        FI->size=atoi(Token);

        ptr=GetToken(ptr,"\\S",&DateStr,0);
        ptr=GetToken(ptr,"\\S",&Token,0);
        DateStr=MCatStr(DateStr," ",Token,NULL);
        ptr=GetToken(ptr,"\\S",&Token,0);
        DateStr=MCatStr(DateStr," ",Token,NULL);

        if (DateStr[4]=='-')
        {
            //full iso format
            FI->mtime=DateStrToSecs("%Y-%m-%d %H:%M:%S.000000000 %Z",DateStr,NULL);
        }
        else
        {
            //standard
            if (strchr(Token,':'))
            {
                DateStr=CatStr(DateStr,":00 ");

                //if it gives a time, it means this year
                DateStr=CatStr(DateStr,GetDateStr("%Y",NULL));
                FI->mtime=DateStrToSecs("%b %d %H:%M:%S %Y",DateStr,NULL);
            }
            else
            {
                //if no time, then will include year instead. Add time
                //or else conversion fails
                DateStr=CatStr(DateStr," 00:00:00");
                FI->mtime=DateStrToSecs("%b %d %Y %H:%M:%S",DateStr,NULL);
            }
        }

        if (FI->type==FTYPE_LINK)
        {
            ptr=GetToken(ptr, "->", &FI->name, 0);
            StripTrailingWhitespace(FI->name);
            while (isspace(*ptr)) ptr++;
            FI->destination=CopyStr(FI->destination, ptr);
        }
        else FI->name=CopyStr(FI->name,ptr);

        FI->path=CopyStr(FI->path,CurrDir);
        FI->path=CatStr(FI->path, FI->name);
    }

    Destroy(Token);
    Destroy(DateStr);
    return(FI);
}



TFileItem *Decode_MLSD_Output(char *CurrDir, char *LsLine)
{
    TFileItem *FI;
    char *FileFacts=NULL;
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    const char *FactNames[]= {"type","modify","perm","size",NULL};
    typedef enum {MLSD_TYPE, MLSD_MTIME, MLSD_PERM,MLSD_SIZE} TMLSD_FIELDS;
    int val;

    if (! StrLen(LsLine)) return(NULL);
    FI=(TFileItem *) calloc(1,sizeof(TFileItem));
    ptr=GetToken(LsLine,"\\S",&FileFacts,0);
    FI->path=CopyStr(FI->path,CurrDir);
    FI->path=CatStr(FI->path,ptr);
    FI->name=CopyStr(FI->name,ptr);


    ptr=GetNameValuePair(FileFacts,";","=",&Name,&Value);
    while (ptr)
    {
        val=MatchTokenFromList(Name,FactNames,0);
        switch (val)
        {
        case MLSD_TYPE:
            if (strcasecmp(Value,"dir")==0) FI->type=FTYPE_DIR;
            else FI->type=FTYPE_FILE;
            break;

        case MLSD_SIZE:
            FI->size=atoi(Value);
            break;

        case MLSD_MTIME:
            FI->mtime=DateStrToSecs("%Y%m%d%H%M%S",Value,NULL);
            break;
        }
        ptr=GetNameValuePair(ptr,";","=",&Name,&Value);
    }

    DestroyString(FileFacts);
    DestroyString(Name);
    DestroyString(Value);

    return(FI);
}

