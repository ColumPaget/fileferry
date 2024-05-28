#include "inet_protocols.h"
#include "../errors_and_logging.h"

//Responses must start with a 3-letter digit, except for POP3 which starts with +OK on success.
//If we don't even get that (so there's silence) we return FALSE here, and the app exits.
int InetReadResponse(STREAM *S, TFileStore *FS, char **ResponseCode, char **Verbiage, int RequiredResponse)
{
    char *Tempstr=NULL, *Sep=NULL, *Terminator=NULL;
    const char *ptr;
    int result=FALSE;

    Tempstr=ReadLoggedLine(Tempstr, FS, S);
    if (! StrValid(Tempstr)) return(FALSE);

    StripCRLF(Tempstr);
    if (StrLen(Tempstr) > 3)
    {
        ptr=GetToken(Tempstr, "-| ", ResponseCode, GETTOKEN_MULTI_SEP | GETTOKEN_INCLUDE_SEP);
        ptr=GetToken(ptr, "-| ", &Sep, GETTOKEN_MULTI_SEP | GETTOKEN_INCLUDE_SEP);
        Terminator=MCopyStr(Terminator, *ResponseCode, " ", NULL);


        switch (**ResponseCode)
        {
//'Response-code' style
        case '1':
            if (RequiredResponse & INET_CONTINUE) result=TRUE;
            break;
        case '2':
            if (RequiredResponse & INET_OKAY) result=TRUE;
            break;
        case '3':
            if (RequiredResponse & INET_MORE) result=TRUE;
            break;

//POP3 style
        case '+':
            if ((strcmp(*ResponseCode,"+OK")==0) && (RequiredResponse & INET_OKAY)) result=TRUE;
            break;

        case '-':
            while ((*ptr !='\0') && (! isspace(*ptr))) ptr++;
            break;
        }

        if (Verbiage) *Verbiage=MCopyStr(*Verbiage, ptr, "\n", NULL);

        if (! isspace(*Sep))
        {
            while (StrValid(Tempstr))
            {
                StripCRLF(Tempstr);
                if (strncmp(Tempstr, Terminator, StrLen(Terminator))==0) break;

                if (Verbiage)
                {
                    if (Tempstr[3]=='-') *Verbiage=MCatStr(*Verbiage,"\n",Tempstr+4,NULL);
                    else *Verbiage=MCatStr(*Verbiage,"\n",Tempstr,NULL);
                }
                Tempstr=ReadLoggedLine(Tempstr, FS, S);
            }
        }
    }

    if (Verbiage && *Verbiage) StripCRLF(*Verbiage);

    Destroy(Terminator);
    Destroy(Tempstr);
    Destroy(Sep);

    return(result);
}
