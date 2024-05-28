#ifndef FILEFERRY_LS_OUTPUT_H
#define FILEFERRY_LS_OUTPUT_H

#include "../fileitem.h"

TFileItem *Decode_LS_Output(char *CurrDir, char *LsLine);
TFileItem *Decode_MLSD_Output(char *CurrDir, char *LsLine);


#endif
