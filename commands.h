#ifndef FILEFERRY_COMMANDS
#define FILEFERRY_COMMANDS

#include "common.h"
#include "filestore.h"

typedef enum {CMD_NONE, CMD_LS, CMD_LLS, CMD_CD, CMD_LCD, CMD_MKDIR, CMD_LMKDIR, CMD_RMDIR, CMD_LRMDIR, CMD_PWD, CMD_LPWD, CMD_GET, CMD_PUT, CMD_MGET, CMD_MPUT, CMD_SHOW, CMD_LSHOW, CMD_SHARE, CMD_DEL, CMD_LDEL, CMD_RENAME, CMD_LRENAME, CMD_COPY, CMD_LCOPY, CMD_LINK, CMD_LLINK, CMD_CHMOD, CMD_LCHMOD, CMD_CHEXT, CMD_LCHEXT, CMD_CRC, CMD_LCRC, CMD_MD5, CMD_LMD5, CMD_SHA1, CMD_LSHA1, CMD_SHA256, CMD_LSHA256, CMD_INFO, CMD_EXISTS, CMD_LEXISTS, CMD_STAT, CMD_LSTAT, CMD_LOCK, CMD_LLOCK, CMD_UNLOCK, CMD_LUNLOCK, CMD_CHPASSWORD, CMD_DIFF, CMD_SET, CMD_HELP, CMD_QUIT} TCmds;

#define CMD_FLAG_ALL             1
#define CMD_FLAG_LONG            2
#define CMD_FLAG_LONG_LONG       4
#define CMD_FLAG_SORT_SIZE       8
#define CMD_FLAG_SORT_TIME      16
#define CMD_FLAG_SYNC           32
#define CMD_FLAG_RECURSE        64
#define CMD_FLAG_FORCE         128
#define CMD_FLAG_ABORT         256
#define CMD_FLAG_QUIT          512
#define CMD_FLAG_FILES_ONLY   4096
#define CMD_FLAG_DIRS_ONLY    8192
#define CMD_FLAG_PAGE        16384
#define CMD_FLAG_WAIT        16384
#define CMD_FLAG_INVERT      32768
#define CMD_FLAG_ENCRYPT     65536
#define CMD_FLAG_IMG        131072
#define CMD_FLAG_SIXEL      262144
#define CMD_FLAG_RESUME     524288

#define ENCRYPT_NONE        0
#define ENCRYPT_ANY         1
#define ENCRYPT_OPENSSL_PW  2
#define ENCRYPT_GPG_PW      3
#define ENCRYPT_7ZIP        4
#define ENCRYPT_ZIP         5

typedef struct
{
    int Type;
    int Flags;
    int EncryptType;
    int NoOfItems;
    char *Mode;
    char *Target;
    char *Dest;
    char *Includes;
    char *Excludes;
    char *EncryptKey;
    ListNode *Vars;
} TCommand;

char *CommandFileLoad(char *RetStr, const char *Path);
int CommandMatch(const char *Str);
TCommand *CommandParse(const char *Str);
void CommandDestroy(void *p_Cmd);
int CommandProcess(TCommand *Cmd, TFileStore *LocalFS, TFileStore *RemoteFS);
void CommandListProcess(const char *Commands, TFileStore *LocalFS, TFileStore *RemoteFS);


void CommandActivateTimeout();
void CommandDeactivateTimeout();


#endif
