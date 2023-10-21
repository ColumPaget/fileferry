#include "help.h"
#include "commands.h"
#include "ui.h"

void HelpCommandList()
{
    printf("Available commands:\n");
    printf("help                 this help\n");
    printf("help <command>       detailed help for <command>\n");
    printf("exit                 quit program\n");
    printf("quit                 quit program\n");
    printf("set                  show settings\n");
    printf("set <name> <value>   change setting '<name>' to '<value>'\n");
    printf("cd <path>            change remote directory to <path>\n");
    printf("chdir <path>         change remote directory to <path>\n");
    printf("lcd <path>           change local directory to <path>\n");
    printf("lchdir <path>        change local directory to <path>\n");
    printf("pwd                  show current remote directory\n");
    printf("lpwd                 show current local directory\n");
    printf("ls <path>            list remote directory. <path> is optional\n");
    printf("lls <path>           list local directory. <path> is optional\n");
    printf("stat <path>          statistics for remote directory. <path> is optional\n");
    printf("lstat <path>         statistics for local directory. <path> is optional\n");
    printf("stats <path>         statistics for remote directory. <path> is optional\n");
    printf("lstats <path>        statistics for local directory. <path> is optional\n");
    printf("mkdir <path>         make a remote directory <path>\n");
    printf("lmkdir <path>        make a local directory <path>\n");
    printf("rm <path>            delete remote file at <path> (wildcards allowed)\n");
    printf("lrm <path>           delete local file at <path> (wildcards allowed)\n");
    printf("del <path>           delete remote file at <path> (wildcards allowed)\n");
    printf("ldel <path>          delete local file at <path> (wildcards allowed)\n");
    printf("chmod <mode> <path>  change mode of files in <path> (wildcards allowed)\n");
    printf("chmod <mode> <path>  change mode of files in <path> (wildcards allowed)\n");
    printf("exists <path>        check if files matching <path> exist (wildcards allowed)\n");
    printf("lexists <path>       check if files matching <path> exist (wildcards allowed)\n");
    printf("get <path>           copy remote file at <path> to local (wildcards allowed)\n");
    printf("put <path>           copy local file at <path> to remote (wildcards allowed)\n");
    printf("share <path>         get 'sharing' url for remote file at <path> (if filestore supports this)\n");
    printf("show <path>          output contents remote file at <path> to screen\n");
    printf("lshow <path>         output contents local file at <path> to screen\n");
    printf("cp <src> <dest>      copy remote file at <src> to remote <dest>\n");
    printf("copy <src> <dest>    copy remote file at <src> to remote <dest>\n");
    printf("lcp <src> <dest>     copy local file at <src> to local <dest>\n");
    printf("lcopy <src> <dest>   copy local file at <src> to local <dest>\n");
    printf("mv <src> <dest>      move/rename remote file at <src> to remote <dest>\n");
    printf("move <src> <dest>    move/rename remote file at <src> to remote <dest>\n");
    printf("rename <src> <dest>  move/rename remote file at <src> to remote <dest>\n");
    printf("lmv <src> <dest>     move/rename local file at <src> to local <dest>\n");
    printf("lmove <src> <dest>   move/rename local file at <src> to local <dest>\n");
    printf("lrename <src> <dest> move/rename local file at <src> to local <dest>\n");
    printf("chext <new extn> <files>  change file extention of remote files to <new extn>. (wildcards allowed)\n");
    printf("lchext <new extn> <files>  change file extention of local files to <new extn>. (wildcards allowed)\n");
    printf("info encrypt         print info about transport encryption on the current connection\n");
    printf("info usage           print info about disk/quota usage\n");
    printf("crc <path>           print checksum for file at <path> on remote host (only on services that support this)\n");
    printf("lcrc <path>          print checksum for file at <path> on local host\n");
    printf("md5 <path>           print md5 hash for file at <path> on remote host (only on services that support this)\n");
    printf("lmd5 <path>          print md5 hash for file at <path> on local host\n");
    printf("sha1 <path>          print sha1 hash for file at <path> on remote host (only on services that support this)\n");
    printf("lsha1 <path>         print sha1 hash for file at <path> on local host\n");
    printf("sha256 <path>        print sha256 hash for file at <path> on remote host (only on services that support this)\n");
    printf("lsha256 <path>       print sha256 hash for file at <path> on local host\n");





    /*
        else if (strcmp(Str, "info")==0) Cmd=CMD_INFO;
        else if (strcmp(Str, "lock")==0) Cmd=CMD_LOCK;
        else if (strcmp(Str, "chpassword")==0) Cmd=CMD_CHPASSWORD;
        else if (strcmp(Str, "password")==0) Cmd=CMD_CHPASSWORD;
        else if (strcmp(Str, "chpasswd")==0) Cmd=CMD_CHPASSWORD;
        else if (strcmp(Str, "passwd")==0) Cmd=CMD_CHPASSWORD;
    */
}

void HelpCommand(const char *Command)
{
    char *Str=NULL;

    GetToken(Command, "\\S", &Str, 0);

    switch (CommandMatch(Str))
    {
    case CMD_INFO:
        printf("info encrypt                print info of encryption of current connection (where known)\n");
        printf("info usage                  print disk usage info of remote host\n");
        printf("info features               print features of remote host\n");
        break;

    case CMD_PWD:
        printf("pwd                         print current working directory on remote host\n");
        break;

    case CMD_LPWD:
        printf("lpwd                        print current working directory on local host\n");
        break;

    case CMD_CD:
        printf("cd <dir>                   change directory to <dir> on remote host\n");
        printf("chdir <dir>                change directory to <dir> on remote host\n");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if chdir fails\n");
        printf("    -A                     (scripts only) abort script, logging an error, if chdir fails\n");
        break;

    case CMD_LCD:
        printf("lcd <dir>                  change directory to <dir> on local host\n");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if chdir fails\n");
        printf("    -A                     (scripts only) abort script, logging an error, if chdir fails\n");
        break;

    case CMD_DEL:
        printf("rm  <path>                 delete file at <path> on remote host\n");
        printf("del <path>                 delete file at <path> on remote host\n");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if delete fails\n");
        printf("    -A                     (scripts only) abort script, logging an error, if delete fails\n");
        break;

    case CMD_LDEL:
        printf("lrm  <path>                 delete file at <path> on local host\n");
        printf("ldel <path>                 delete file at <path> on local host\n");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if delete fails\n");
        printf("    -A                     (scripts only) abort script, logging an error, if delete fails\n");
        break;

    case CMD_MKDIR:
        printf("mkdir <path>               make directory at <path> on remote host\n");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if mkdir fails\n");
        printf("    -A                     (scripts only) abort script, logging an error, if mkdir fails\n");
        break;

    case CMD_LMKDIR:
        printf("lmkdir <path>              make directory at <path> on local host\n");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if mkdir fails\n");
        printf("    -A                     (scripts only) abort script, logging an error, if mkdir fails\n");
        break;


    case CMD_EXISTS:
        printf("exists <pattern>           check if items matching '<pattern>' exist on remote host. Prints no output, used in scripts with the options -A or -Q");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if no matching files\n");
        printf("    -A                     (scripts only) abort script, logging an error, if no matching files\n");
        break;

    case CMD_LEXISTS:
        printf("lexists <pattern>           check if items matching '<pattern>' exist on remote host. Prints no output, used in scripts with the options -A or -Q");
        printf("options:\n");
        printf("    -Q                     (scripts only) quit script silently if no matching files\n");
        printf("    -A                     (scripts only) abort script, logging an error, if no matching files\n");
        break;

    case CMD_LS:
        printf("ls <path>              	   list files and folders on remote host\n");
        printf("options:\n");
        printf("    -l                     list more info (file size and last update)\n");
        printf("    -ll                    list even more info\n");
        printf("    -F                     force directory reload rather than using cached directory listing\n");
        printf("    -n <num>               show only '<num>' items\n");
        printf("    -S                     sort listing by file size\n");
        printf("    -t                     sort listing by timestamp\n");
        printf("    -page                  break listing up into pages\n");
        printf("    -pg                    break listing up into pages\n");
        printf("    -f                     list files only\n");
        printf("    -files                 list files only\n");
        printf("    -d                     list directories only\n");
        printf("    -dirs                  list directories only\n");
        printf("    -newer <when>          list items newer than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -older <when>          list items older than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -smaller <size>        list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        printf("    -larger <size>         list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        break;

    case CMD_LLS:
        printf("lls <path>             	   list files and folders on local host\n");
        printf("options:\n");
        printf("    -l                     list more info (file size and last update)\n");
        printf("    -ll                    list even more info\n");
        printf("    -F                     force directory reload rather than using cached directory listing\n");
        printf("    -n <num>               show only '<num>' items\n");
        printf("    -S                     sort listing by file size\n");
        printf("    -t                     sort listing by timestamp\n");
        printf("    -page                  break listing up into pages\n");
        printf("    -pg                    break listing up into pages\n");
        printf("    -f                     list files only\n");
        printf("    -files                 list files only\n");
        printf("    -d                     list directories only\n");
        printf("    -dirs                  list directories only\n");
        printf("    -newer <when>          list items newer than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -older <when>          list items older than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -smaller <size>        list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        printf("    -larger <size>         list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        break;

    case CMD_STAT:
        printf("stats <path>               statistics for files and folders on remote host\n");
        printf("options:\n");
        printf("    -F                     force directory reload rather than using cached directory listing\n");
        printf("    -newer <when>          list items newer than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -older <when>          list items older than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -smaller <size>        list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        printf("    -larger <size>         list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        break;


    case CMD_LSTAT:
        printf("lstats <path>             statistics for files and folders on local host\n");
        printf("options:\n");
        printf("    -F                     force directory reload rather than using cached directory listing\n");
        printf("    -newer <when>          list items newer than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -older <when>          list items older than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -smaller <size>        list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        printf("    -larger <size>         list items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        break;


    case CMD_GET:
    case CMD_MGET:
        printf("put <options> <path>     pull a file from remote host to local host (expects one file as argument)\n");
        printf("mput <options> <path>    pull a file from remote host to local host (expects multiple files as arguments)\n");
        printf("options:\n");
        printf("    -f                   force transfer even if destination already exists\n");
        printf("    -F                   force transfer even if destination already exists\n");
        printf("    -s                   sync. Only transfer files newer than existing files\n");
        printf("    -n <number>          only transfer 'number' files of those matching request\n");
        printf("    -t <extn>            when transferring files, send them with temporary file extension <extn> and then rename to proper name/extension when transfer is complete\n");
        printf("    -et <extn>           when transferring files, send them with temporary file extension <extn> and then rename to proper name/extension when transfer is complete\n");
        printf("    -ed <extn>           change file extension of destination file to  <extn> when transfer is complete\n");
        printf("    -es <extn>           change file extension of source file to  <extn> when transfer is complete\n");
        printf("    -Ed <extn>           append <extn> to file extension of destination file when transfer is complete\n");
        printf("    -Es <extn>           append <extn> to file extension of source file when transfer is complete\n");
        printf("    -enc <config>        dencrypt file using config '<config>' before receiving\n");
        printf("    -h <script>          'hook' script to run when transfer is complete\n");
        printf("    -posthook <script>   'hook' script to run when transfer is complete\n");
        printf("    -prehook <script>    'hook' script to run just before transfer starts\n");
        printf("    -bak                 make a backup of destination files before overwritting them\n");
        printf("    -x <pattern>         exclude files matching pattern\n");
        printf("    -i <pattern>         include files matching pattern\n");
        printf("    -newer <when>        transfer files newer than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -older <when>        transfer files older than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -smaller <size>      transfer items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        printf("    -larger <size>       transfer items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        break;


    case CMD_PUT:
    case CMD_MPUT:
        printf("put <options> <path>     push a file from local host to remote host (expects one file as argument)\n");
        printf("mput <options> <path>    push a file from local host to remote host (expects multiple files as arguments)\n");
        printf("options:\n");
        printf("    -f                   force transfer even if destination already exists\n");
        printf("    -F                   force transfer even if destination already exists\n");
        printf("    -s                   sync. Only transfer files newer than existing files\n");
        printf("    -n <number>          only transfer 'number' files of those matching request\n");
        printf("    -t <extn>            when transferring files, send them with temporary file extension <extn> and then rename to proper name/extension when transfer is complete\n");
        printf("    -et <extn>           when transferring files, send them with temporary file extension <extn> and then rename to proper name/extension when transfer is complete\n");
        printf("    -ed <extn>           change file extension of destination file to  <extn> when transfer is complete\n");
        printf("    -es <extn>           change file extension of source file to  <extn> when transfer is complete\n");
        printf("    -Ed <extn>           append <extn> to file extension of destination file when transfer is complete\n");
        printf("    -Es <extn>           append <extn> to file extension of source file when transfer is complete\n");
        printf("    -enc                 encrypt file before sending, or decrypt on recieving, using any encryption method available\n");
        printf("    -ssl                 encrypt file before sending, or decrypt on recieving, using openssl encryption\n");
        printf("    -sslenc              encrypt file before sending, or decrypt on recieving, using openssl encryption\n");
        printf("    -gpg                 encrypt file before sending, or decrypt on recieving, using gpg with a password\n");
        printf("    -zenc                encrypt file before sending, or decrypt on recieving, using zip with a password\n");
        printf("    -7zenc               encrypt file before sending, or decrypt on recieving, using 7zip with a password\n");
        printf("    -key <key>           use <key> as encryption key/password\n");
        printf("    -h <script>          'hook' script to run when transfer is complete\n");
        printf("    -posthook <script>   'hook' script to run when transfer is complete\n");
        printf("    -prehook <script>    'hook' script to run just before transfer starts\n");
        printf("    -bak                 make a backup of destination files before overwritting them\n");
        printf("    -x <pattern>         exclude files matching pattern\n");
        printf("    -i <pattern>         include files matching pattern\n");
        printf("    -newer <when>        transfer files newer than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -older <when>        transfer files older than <when>. <when> can be a duration, e.g. '7d' '2w' '1y' or a date in format YYYY/mm/dd or a time in format HH:MM:SS or a combined date/time in format YYYY/mm/ddTHH:MM:SS\n");
        printf("    -smaller <size>      transfer items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        printf("    -larger <size>       transfer items smaller than <size>, where <size> is a number with a suffix: e.g. 200k, 4M, 1G\n");
        break;

    case CMD_RENAME:
        printf("mv <options> <src> <dst>    move/rename files from <src> to <dst> on remote host\n");
        break;

    case CMD_LRENAME:
        printf("lmv <options> <src> <dst>   move/rename files from <src> to <dst> on local host\n");
        break;

    case CMD_COPY:
        printf("cp <options> <src> <dst>    copy files from <src> to <dst> on remote host\n");
        break;

    case CMD_LCOPY:
        printf("lcp <options> <src> <dst>   copy files from <src> to <dst> on local host\n");
        break;

    case CMD_SHOW:
        printf("show <path>                 display contents of file at <path> on remote host\n");
        printf("    -img                    show image files using an image viewer or terminal sixel support\n");
        printf("    -image                  show image files using an image viewer or terminal sixel support\n");
        break;

    case CMD_LSHOW:
        printf("lshow <path>                display contents of file at <path> on local host\n");
        printf("    -img                    show image files using an image viewer or terminal sixel support\n");
        printf("    -image                  show image files using an image viewer or terminal sixel support\n");
        break;

    case CMD_SHARE:
        printf("share <path>                generate 'sharing url' for file at <path> on hosts/services that support this feature\n");
        break;

    case CMD_CRC:
        printf("crc <path>                  generate CRC checksum for file at <path> on hosts/services that support this feature\n");
        break;

    case CMD_LCRC:
        printf("lcrc <path>                 generate CRC checksum for file at <path> on localhost\n");
        break;

    case CMD_MD5:
        printf("md5 <path>                  generate MD5 hash for file at <path> on hosts/services that support this feature\n");
        break;

    case CMD_LMD5:
        printf("lmd5 <path>                 generate MD5 hash for file at <path> on localhost\n");
        break;

    case CMD_SHA1:
        printf("sha1 <path>                 generate SHA1 hash for file at <path> on hosts/services that support this feature\n");
        break;

    case CMD_LSHA1:
        printf("lsha1 <path>                generate SHA1 hash for file at <path> on localhost\n");
        break;

    case CMD_SHA256:
        printf("sha256 <path>               generate SHA256 hash for file at <path> on hosts/services that support this feature\n");
        break;

    case CMD_LSHA256:
        printf("lsha256 <path>              generate SHA256 hash for file at <path> on localhost\n");
        break;

    case CMD_LOCK:
        printf("lock <path>                 lock a file for filestores that support this (currently only local disk)\n");
        printf("    -w                      wait to obtain lock, rather than giving up if file locked\n");
        printf("    -wait                   wait to obtain lock, rather than giving up if file locked\n");
        break;

    case CMD_LLOCK:
        printf("llock <path>                lock a file on local disk\n");
        printf("    -w                      wait to obtain lock, rather than giving up if file locked\n");
        printf("    -wait                   wait to obtain lock, rather than giving up if file locked\n");
        break;

    default:
        HelpCommandList();
        break;
    }

    printf("\n");

    Destroy(Str);
}


void HelpCommandLine()
{
    printf("usage:\n");
    printf("  fileferry [options] [url]                  - connect to a filestore by url\n");
    printf("  fileferry [options] [name]                 - connect to a saved, filestore by name\n");
    printf("  fileferry -add [name] [filestore url]      - add a filestore to saved filestores list\n");
    printf("  fileferry -filestores                      - print list of saved filestores\n");
    printf("  fileferry -drivers                         - print list of available filestore drivers\n");
    printf("options:\n");
    printf("  -v                                         - be verbose\n");
    printf("  -N                                         - batch mode, do not try to read any user input\n");
    printf("  -D                                         - print debugging\n");
    printf("  -debug                                     - print debugging\n");
    printf("  -f <path>                                  - path to config file\n");
    printf("  -conf <path>                               - path to config file\n");
    printf("  -fsfile <path>                             - path to saved filestores file\n");
    printf("  -l <path>                                  - log to file at <path>\n");
    printf("  -log <path>                                - log to file at <path>\n");
    printf("  -user <username>                           - username to log in with\n");
    printf("  -password <password>                       - password to log in with\n");
    printf("  -passwd <password>                         - password to log in with\n");
    printf("  -pass <password>                           - password to log in with\n");
    printf("  -pw <password>                             - password to log in with\n");
    printf("  -c <commands>                              - list of commands to run instead of reading commands from user\n");
    printf("  -b <command file>                          - path to a file containing commands to run instead of reading commands from user\n");
    printf("  -e <config>                                - encrypt/decrypt transferred files\n");
    printf("  -i <path>                                  - path to identity file for sftp, or certificate for webdav/ftps\n");
    printf("  -proxy <url>                               - use proxy at <url>\n");
    printf("  -timeout <timeout>                         - interrupt a command after <timeout>\n");
    printf("  -life <timeout>                            - shutdown program after <timeout>\n");
    printf("  -errors-email <email address>              - send email report of any errors to <email address>\n");
    printf("  -smtp-server <url>                         - send error report email via smtp server at <url>\n");
    printf("  -nols                                      - don't get directory listing when changing directory\n");

    UI_Close();
    exit(0);
}
