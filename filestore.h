#ifndef FILEFERRY_FILESTORE_H
#define FILEFERRY_FILESTORE_H

#include "common.h"
#include "fileitem.h"
#include <sys/stat.h>

#define FILESTORE_DEBUG 1
#define FILESTORE_ATTACHED 2
#define FILESTORE_CONNECTED 4
#define FILESTORE_TLS 32
#define FILESTORE_NOPATH 64
#define FILESTORE_FOLDERS 1024
#define FILESTORE_USAGE 2048
#define FILESTORE_SHARELINK 4096

// we do not use 'setuid', 'setgid' or 'set sticky bit' in mkdir so we can
// use these flags for other things
#define MKDIR_EXISTS_OKAY S_ISUID

typedef enum {FILESTORE_FTP, FILESTORE_SFTP, FILESTORE_HTTP, FILESTORE_WEBDAV} EFileStoreTypes;

typedef struct t_struct_fs TFileStore;
typedef int (*CONNECT_FUNC)(TFileStore *FS);
typedef int (*RENAME_FUNC)(TFileStore *FS, const char *From, const char *To);
typedef int (*PATH_FUNC)(TFileStore *FS, const char *Path);
typedef int (*ITEM_FUNC)(TFileStore *FS, TFileItem *FI);
typedef int (*MKDIR_FUNC)(TFileStore *FS, const char *Path, int MkDir);
typedef int (*CHMOD_FUNC)(TFileStore *FS, const char *Path, int Mode);
typedef char *(*GETVALUE_FUNC)(char *Buffer, TFileStore *FS, const char *Path, const char *Value);
typedef ListNode *(*LIST_DIR_FUNC)(TFileStore *FS, const char *URL);
typedef STREAM *(*OPENFILE_FUNC)(TFileStore *FS, const char *URL, const char *OpenType, uint64_t size);
typedef int (*READFILE_FUNC)(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len);
typedef int (*WRITEFILE_FUNC)(TFileStore *FS, STREAM *S, char *Buffer, uint64_t offset, uint32_t len);
typedef int (*CLOSEFILE_FUNC)(TFileStore *FS, STREAM *S);
typedef TFileItem *(*FILEINFO_FUNC)(TFileStore *FS, const char *Path);

typedef struct t_struct_fs
{
    int Type;
    int Flags;
    char *Name;
    char *URL;
    char *Proxy;
    char *User;
    char *Pass;
    char *AuthCreds;
    char *Features;
    char *Encrypt;
    char *CredsFile;
    CONNECT_FUNC Connect;
    CONNECT_FUNC DisConnect;
    LIST_DIR_FUNC ListDir;
    OPENFILE_FUNC OpenFile;
    CLOSEFILE_FUNC CloseFile;
    READFILE_FUNC ReadBytes;
    WRITEFILE_FUNC WriteBytes;
    RENAME_FUNC RenamePath;
    RENAME_FUNC CopyPath;
    PATH_FUNC ChDir;
    PATH_FUNC UnlinkPath;
    CHMOD_FUNC ChMod;
    CHMOD_FUNC Lock;
    PATH_FUNC UnLock;
    PATH_FUNC Freeze;
    FILEINFO_FUNC FileInfo;
    MKDIR_FUNC MkDir;
    GETVALUE_FUNC GetValue;
    RENAME_FUNC ChPassword;
    char *CurrDir;
    char *HomeDir;
    char *Handle;
    STREAM *S;
    ListNode *DirList;
    ListNode *Vars;
    time_t TimeOffset;
    uint64_t AvailStorage;
} TSF;


TFileStore *FileStoreCreate();
void FileStoreDestroy(void *p_FileStore);
TFileStore *FileStoreParse(const char *Config);
TFileStore *FileStoreConnect(const char *URL);
void FileStoreDisConnect(TFileStore *FS);
const char *FileStorePathRelativeToCurr(TFileStore *FS, const char *Path);
char *FileStoreFullURL(char *RetStr, const char *Target, TFileStore *FS);
char *FileStoreReformatPath(char *RetStr, const char *Path, TFileStore *FS);
ListNode *FileStoreGlob(TFileStore *FS, const char *Path);
ListNode *FileStoreGetDirList(TFileStore *FS, const char *Target);
int FileStoreGlobCount(TFileStore *FS, const char *Path);
int FileStoreItemExists(TFileStore *FS, const char *FName);
int FileStoreChDir(TFileStore *FS, const char *Path);
int FileStoreMkDir(TFileStore *FS, const char *Path, int Mode);
int FileStoreUnlinkPath(TFileStore *FS, const char *Path);
int FileStoreUnlinkItem(TFileStore *FS, TFileItem *FI);
int FileStoreRename(TFileStore *FS, const char *Path, const char *Dest);
int FileStoreChMod(TFileStore *FS, const char *Mode, const char *Target);
int FileStoreLock(TFileStore *FS, const char *Path, int Flags);
int FileStoreUnLock(TFileStore *FS, const char *Path);
int FileStoreChPassword(TFileStore *FS, const char *OldPassword, const char *NewPassword);
char *FileStoreGetValue(char *RetStr, TFileStore *FS, const char *Path, const char *Value);
void FileStoreAddItem(TFileStore *FS, int Type, const char *Name, uint64_t Size);
void FileStoreDeleteItem(TFileStore *FS, const char *Path);
int FileStoreCopyFile(TFileStore *FS, const char *Path, const char *Dest);
TFileItem *FileStoreGetFileInfo(TFileStore *FS, const char *Path);
void FileStoreOutputDiskQuota(TFileStore *FS);
void FileStoreOutputSupportedFeatures(TFileStore *FS);
void FileStoreRecordCipherDetails(TFileStore *FS, STREAM *S);
void FileStoreOutputCipherDetails(TFileStore *FS, int Verbosity);

void FileStoreFreeDir(TFileStore *FS, ListNode *Dir);

#endif
