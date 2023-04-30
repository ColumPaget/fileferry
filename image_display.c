#include "image_display.h"
#include "settings.h"

static char *FindProgram(char *RetStr, const char *Candidates)
{
    char *FileName=NULL, *Candidate=NULL;
    const char *p_Args, *ptr;
    int i;

    RetStr=CopyStr(RetStr, "");
    ptr=GetToken(Candidates, ",", &Candidate, 0);
    while (ptr)
    {
        p_Args=GetToken(Candidate, "\\S", &FileName, 0);
        RetStr=FindFileInPath(RetStr, FileName, getenv("PATH"));
        if (StrValid(RetStr))
        {
            RetStr=MCatStr(RetStr, " ", p_Args, NULL);
            break;
        }
        ptr=GetToken(ptr, ",", &Candidate, 0);
    }

    Destroy(FileName);
    Destroy(Candidate);

    return(RetStr);
}


static char *SelectProgram(char *RetStr, const char *Candidates, const char *ImagePath)
{
    char *Tempstr=NULL;
    ListNode *Vars;
    char *ptr;
    int val;

    Vars=ListCreate();

    val=strtol(Settings->ImagePreviewSize, &ptr, 10);
    if (*ptr != '\0') ptr++;
    Tempstr=FormatStr(Tempstr, "%d", val);
    SetVar(Vars, "width", Tempstr);

    val=strtol(ptr, NULL, 10);
    Tempstr=FormatStr(Tempstr, "%d", val);
    SetVar(Vars, "height", Tempstr);

    SetVar(Vars, "path", ImagePath);

    RetStr=CopyStr(RetStr, "");
    Tempstr=FindProgram(Tempstr, Candidates);
    RetStr=SubstituteVarsInString(RetStr, Tempstr, Vars, 0);

    ListDestroy(Vars, Destroy);
    Destroy(Tempstr);

    return(RetStr);
}


void DisplayImage(TCommand *Cmd, const char *Title, const char *ImagePath)
{
    char *Tempstr=NULL;
    pid_t pid;

    if (Cmd->Flags & CMD_FLAG_SIXEL)
    {
        printf("%s\n", Title);
        Tempstr=SelectProgram(Tempstr, Settings->Sixelers, ImagePath);
        system(Tempstr);
        printf("\n");
        unlink(ImagePath);
    }
    else
    {
        Tempstr=SelectProgram(Tempstr, Settings->ImageViewers, ImagePath);
        if (StrValid(Tempstr))
        {
            pid=xfork("outnull");
            if (pid==0)
            {
                system(Tempstr);
                unlink(ImagePath);
                _exit(0);
            }
        }
    }

    Destroy(Tempstr);
}
