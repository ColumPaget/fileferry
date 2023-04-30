#include "password.h"
#include "settings.h"
#include "ui.h"


/*
int PasswordGet(TFileStore *FS, int Try, const char **Pass)
{
    static char *Tempstr=NULL;
    int result;

    if (StrValid(FS->Pass)) CredsStoreAdd(FS->URL, FS->User, FS->Pass);
    result=CredsStoreLookup(FS->URL, FS->User, Pass);
    if (! result)
    {
        Tempstr=UI_AskPassword(Tempstr);
        CredsStoreAdd(FS->URL, FS->User, Tempstr);
    }
    result=CredsStoreLookup(FS->URL, FS->User, Pass);

//is static
//Destroy(Tempstr);

    return(result);
}
*/



int PasswordGet(TFileStore *FS, int Try, const char **Pass)
{
    static char *Tempstr=NULL;

    *Pass=NULL;

    if ( (Try == 0) && StrValid(FS->Pass) )
    {
        *Pass=FS->Pass;
    }
    else if (! (Settings->Flags & SETTING_BATCH))
    {
        Tempstr=UI_AskPassword(Tempstr);
        *Pass=Tempstr;
    }

    return(StrLen(*Pass));
}
