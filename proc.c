#include "proc.h"

char *ProcLookupParent(char *RetStr)
{
    pid_t ppid;
    char *Tempstr=NULL, *Path=NULL;
    int result;

    ppid=getppid();
    Tempstr=FormatStr(Tempstr, "/proc/%d/exe", ppid);
    Path=SetStrLen(Path, PATH_MAX);
    result=readlink(Tempstr, Path, PATH_MAX);
    StrTrunc(Path, result);

    RetStr=FormatStr(RetStr, "%d:%s", ppid, Path);

    Destroy(Tempstr);
    Destroy(Path);

    return(RetStr);
}
