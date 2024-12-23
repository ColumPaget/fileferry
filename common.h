#ifndef FILEFERRY_COMMON_H
#define FILEFERRY_COMMON_H

#include "libUseful-5/libUseful.h"

#define ERR_FULL      -1
#define ERR_FORBID    -2
#define ERR_NOEXIST   -3
#define ERR_TOOBIG    -4
#define ERR_BADNAME   -5
#define ERR_NOCONNECT -6
#define ERR_EXISTS -7

#define EXTN_CHANGE 0
#define EXTN_APPEND 1

#define VERSION "3.1"

extern STREAM *StdIO;
extern uint64_t ProcessStartTime;
extern time_t Now;


void AppendVar(ListNode *Vars, const char *VarName, const char *Value);
void ClipExtension(char *Path);
char *CopyPathChangeExtn(char *RetStr, const char *Path, const char *NewExtn, int Flags);
int FileInPatternList(const char *Path, const char *PatternList);
char *DatePartialToFull(char *RetStr, const char *DateStr);

#endif
