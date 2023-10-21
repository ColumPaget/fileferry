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
    time_t When;
    int val;

    //if no DateStr then use today's date
    if (! StrValid(DateStr)) return(CopyStr(RetStr, GetDateStr("%Y/%m/%d %H:%M:%S", NULL)));

    //if a duration in the form 3d or 2h etc, then return that much time in the past
    val=strtol(DateStr, (char *) &ptr, 10);
    if (val && (StrLen(ptr) ==1))
    {
        When=time(NULL) - ParseDuration(DateStr);
        return(CopyStr(RetStr, GetDateStrFromSecs("%Y/%m/%d %H:%M:%S", When, NULL)));
    }

    //otherwise treat it as a date or time
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
