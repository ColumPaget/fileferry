#include "common.h"
#include "settings.h"
#include "ui.h"
#include <fnmatch.h>

uint64_t ProcessStartTime;
time_t Now;

void ClipExtension(char *Path)
{
    const char *ptr;
    const char *clip=NULL;

    for (ptr=GetBasename(Path); *ptr !='\0'; ptr++)
    {
        if (*ptr=='.') clip=(char *) ptr;
    }

    if (clip) StrTrunc(Path, clip-Path);
}

char *CopyPathChangeExtn(char *RetStr, const char *Path, const char *NewExtn, int Flags)
{
    const char *ptr;

    RetStr=CopyStr(RetStr, Path);
    if (! (Flags & EXTN_APPEND)) ClipExtension(RetStr);

    ptr=NewExtn;
    if (*ptr=='.') ptr++;
    RetStr=MCatStr(RetStr, ".", ptr, NULL);
    return(RetStr);
}


int FileInPatternList(const char *Path, const char *PatternList)
{
    char *Pattern=NULL;
    const char *ptr;
    int RetVal=FALSE;

    ptr=GetToken(PatternList, " ", &Pattern, GETTOKEN_QUOTES);
    while (ptr)
    {
        if (
            (fnmatch(Pattern, Path, 0)==0) ||
            (fnmatch(Pattern, GetBasename(Path), 0)==0)
        )
        {
            RetVal=TRUE;
            break;
        }
        ptr=GetToken(ptr, " ", &Pattern, GETTOKEN_QUOTES);
    }

    Destroy(Pattern);

    return(RetVal);
}


char *DatePartialToFull(char *RetStr, const char *DateStr)
{
    char *Token=NULL, *Tempstr=NULL;
    const char *ptr;

    if (! StrValid(DateStr)) RetStr=CopyStr(RetStr, GetDateStr("%Y/%m/%d %H:%M:%S", NULL));

    ptr=GetToken(DateStr, "\\S|T", &Token, GETTOKEN_QUOTES | GETTOKEN_MULTI_SEP);

    if (StrValid(ptr)) RetStr=CopyStr(RetStr, DateStr);
    else
    {
        if (strchr(Token, ':')) Tempstr=MCopyStr(Tempstr, GetDateStr("%Y/%m/%d", NULL), " ", ptr, NULL);
        else Tempstr=MCopyStr(Tempstr, ptr, " 00:00:00", NULL);
    }

    Destroy(Token);
    Destroy(Tempstr);

    return(RetStr);
}
