#include "filestore_drivers.h"
#include "localdisk.h"

static ListNode *Drivers=NULL;


#ifdef FILESTORE_BUILTIN_HTTP
#include "http.h"
#endif

#ifdef FILESTORE_BUILTIN_FTP
#include "ftp.h"
#endif

#ifdef FILESTORE_BUILTIN_SFTP
#include "sftp.h"
#endif

#ifdef FILESTORE_BUILTIN_POP3
#include "pop3.h"
#endif

#ifdef FILESTORE_BUILTIN_GDRIVE
#include "gdrive.h"
#endif

#ifdef FILESTORE_BUILTIN_DROPBOX
#include "dropbox.h"
#endif

#ifdef FILESTORE_BUILTIN_GOFILE
#include "gofile.h"
#endif

#ifdef FILESTORE_BUILTIN_FILEBIN
#include "filebin.h"
#endif

#ifdef FILESTORE_BUILTIN_FILESANYWHERE
#include "filesanywhere.h"
#endif

#ifdef FILESTORE_BUILTIN_SYNOLOGY
#include "synology.h"
#endif



void FileStoreDriversInit()
{
    FileStoreDriverAdd("file", "Local disk", "Access files on local disk", LocalDisk_Attach);

#ifdef FILESTORE_BUILTIN_HTTP
    FileStoreDriverAdd("http", "HTTP", "HTTP with NO encryption. Uses webdav if server supports it.", HTTP_Attach);
    FileStoreDriverAdd("https", "HTTPS", "SECURE HTTP with SSL/TLS encryption. Uses webdav if server supports it.", HTTP_Attach);
#endif

#ifdef FILESTORE_BUILTIN_FTP
    FileStoreDriverAdd("ftp", "FTP", "Old-school File Transfer Protocol (no encryption)", FTP_Attach);
    FileStoreDriverAdd("ftps", "FTPS", "Old-school File Transfer Protocol with SSL/TLS encryption", FTP_Attach);
#endif

#ifdef FILESTORE_BUILTIN_SFTP
    FileStoreDriverAdd("sftp", "SFTP", "OpenSSH SFTP file transfer", SFTP_Attach);
#endif

#ifdef FILESTORE_BUILTIN_POP3
    FileStoreDriverAdd("pop3", "POP3", "POP3 Mail", POP3_Attach);
    FileStoreDriverAdd("pop3s", "POP3", "POP3 Mail", POP3_Attach);
#endif

#ifdef FILESTORE_BUILTIN_GDRIVE
    FileStoreDriverAdd("gdrive", "Google Drive", "Google drive api v3", GDrive_Attach);
#endif

#ifdef FILESTORE_BUILTIN_GDRIVE
    FileStoreDriverAdd("dropbox", "Dropbox", "DropBox", DropBox_Attach);
#endif

#ifdef FILESTORE_BUILTIN_GOFILE
    FileStoreDriverAdd("gofile", "GoFile", "GoFile API. https://gofile.io/", GoFile_Attach);
#endif

#ifdef FILESTORE_BUILTIN_FILEBIN
    FileStoreDriverAdd("filebin", "FileBin", "FileBin API. https://filebin.net/", FileBin_Attach);
#endif

#ifdef FILESTORE_BUILTIN_FILESANYWHERE
    FileStoreDriverAdd("faw", "FilesAnywhere", "FilesAnywhere v2 API. https://filesanywhere.com ", FilesAnywhere_Attach);
#endif

#ifdef FILESTORE_BUILTIN_SYNOLOGY
    FileStoreDriverAdd("syno", "SynologyNAS", "Synology NAS API.", SYNO_Attach);
#endif



}


void FileStoreDriverAdd(const char *Protocol, const char *Name, const char *Description, CONNECT_FUNC Attach)
{
    TFileStoreDriver *Driver;

    if (! Drivers) Drivers=ListCreate();

    Driver=(TFileStoreDriver *) calloc(1, sizeof(TFileStoreDriver));
    Driver->Name=CopyStr(Driver->Name, Name);
    Driver->Description=CopyStr(Driver->Description, Description);
    Driver->Attach=Attach;

    ListAddNamedItem(Drivers, Protocol, Driver);
}



TFileStoreDriver *FileStoreDriverFind(const char *Protocol)
{
    ListNode *Node;

    Node=ListFindNamedItem(Drivers, Protocol);
    if (Node) return((TFileStoreDriver *) Node->Item);

    return(NULL);
}


int FileStoreDriverAttach(const char *Protocol, TFileStore *FS)
{
    TFileStoreDriver *Driver;

    Driver=FileStoreDriverFind(Protocol);
    if (Driver) return(Driver->Attach(FS));
    return(FALSE);
}


ListNode *FileStoreDriversList()
{
    return(Drivers);
}
