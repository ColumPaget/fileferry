AUTHOR
======

fileferry is (C) 2023 Colum Paget. They are released under the Gnu Public License so you may do anything with them that the GPL allows.

Email: colums.projects@gmail.com

DISCLAIMER
==========

This is free software. It comes with no guarentees and I take no responsiblity if it makes your computer explode or opens a portal to the demon dimensions, or does anything at all, or doesn't.


LICENSE
=======

fileferry is released under the GPLv3.


SYNOPSIS
========

fileferry is a file transfer application with a particular focus on unattended, automated transfer. It currently supports ftp, ftps, sftp, webdav, http, https, https://dropbox.com, https://drive.google.com, https://filesanywhere.com, https://filebin.net and https://gofile.io


INSTALL
=======


```
./configure
make
make install
```

USAGE
=====

```
usage:
  fileferry [options] [url]                  - connect to a filestore by url
  fileferry [options] [name]                 - connect to a saved, filestore by name
  fileferry -add [name] [filestore url]      - add a filestore to saved filestores list
  fileferry -filestores                      - print list of saved filestores
  fileferry -drivers                         - print list of available filestore drivers
options:
  -v                                         - be verbose
  -N                                         - batch mode, do not try to read any user input
  -D                                         - print debugging
  -debug                                     - print debugging
  -f <path>                                  - path to config file
  -conf <path>                               - path to config file
  -fsfile <path>                             - path to saved filestores file
  -l <path>                                  - log to file at <path>
  -log <path>                                - log to file at <path>
  -user <username>                           - username to log in with
  -password <password>                       - password to log in with
  -passwd <password>                         - password to log in with
  -pass <password>                           - password to log in with
  -pw <password>                             - password to log in with
  -c <commands>                              - list of filestore commands to run instead of reading commands from user
  -b <command file>                          - read a list of commands from a 'batch' command file
  -e <config>                                - encrypt/decrypt transferred files
  -i <path>                                  - path to identity file for sftp, or certificate for webdav/ftps
  -proxy <url>                               - use proxy at <url>
  -timeout <timeout>                         - interrupt a command after <timeout>
  -life <timeout>                            - shutdown program after <timeout>
  -errors-email <email address>              - send email report of any errors to <email address>
  -smtp-server <url>                         - send error report email via smtp server at <url>
  -nols                                      - don't grab a directory listing every time we change directory
```


URL TYPES
=========

The currently supported list of URL types for fileferry is:


```
file:            local disk urls. You wouldn't usually use this
sftp:            SFTP, over SSH, openssh must be installed
ftp:             Plain old FTP
ftps:            FTP over SSL/TLS
http:            HTTP (web) also webdav if the site supports it
https:           HTTPS(web) also webdav if the site supports it
pop3:            POP3 mail protocol
pop3s:           POP3 mail protocol over SSL/TLS
gdrive:          google drive
dropbox:         drop box
gofile:          gofile.io
filebin:         filebin.net
faw:             filesanywhere.com
```

Of these the sftp, ftp, ftps http and https URL/filestore types are the most tested, the others are all somewhat experimental.


FILESTORES
==========

fileferry has a concept of 'filestores', these are simply sites or servers that store files. You can connect to these by giving a full URL, possibly including autentication details, on the command-line. However, you can also save filestore details in a 'filestores file' and then refer to them by an alias.

Filestores are saved using the `-add` option:

```
  fileferry -add mydrive gdrive:user1@gdrive
  fileferry -add admin-server https:fred:password1234@webdav.test.com:8443
```

these can then be used as:

```
fileferry mydrive
fileferry admin-server
```

The filestores file is, by default, stored in `~/.config/fileferry/filestores.conf` but the `-fsfile` command line option can be used to specify an alternative file.



WEBDAV FILESTORES
=================

Webdav support brings certain issues with it. Some sites mix webdav access, behind a username and password, with 'public' access to other pages without authentication. For instance, a podcast feed might support webdav with authentication so the feed provider can update it. Thus the same URL can provide different services. In order to distinguish between webdav and straight web-pages fileferry  will will attempt to login to any url where a username is provided in the url.

Webdav has been seen to work with the following providers:

MyDrive.ch    https://www.mydrive.ch/
DriveHQ       https://drivehq.com
Koofr         https://koofr.net
Yandex        https://yandex.com
opendrive     https://opendrive.com
woelkli       https://woelkli.com
4shared       https://4shared.com
powerfolder   https://powerfolder.com

RSS FILESTORES
==============

fileferry supports RSS/atom feeds and extracts filenames from these feeds when they are encountered, allowing you to view a podcast feed as though it were an FTP/webdav server.



SFTP FILESTORES 
===============

SFTP filestores are a special case. SSH/SFTP has a feature where server details can be stored against and alieas in the '~/.ssh/config' file. Thus sftp connections can be specified to fileferry using:

1) a full url
2) an ssh 'config' alias
3) a fileferry filestore alias that references either a url, or an ssh 'config' alias

SFTP filestores require openssh (though not the sftp program) installed to work.



PROXY SUPPORT
=============

fileferry can be instructed to use a proxy server via either the `-proxy` command-line option, or the `PROXY_SERVER` environment variable. 



TRANSFER FEATURES
=================

fileferry is designed mostly for automated transfers. A common problem is sending files to a system where an automated process will grab the file as soon as it appears, as this can result in files being imported when they only partially transferred. In order to help with this, fileferry has the ability to transfer files with a '.tmp' extension, and then rename them to their true file extension when the file is transferred. In it's most basic form this is using the `-t` option for the put command:

```
put -t myfile.txt
```

This will cause 'myfile.txt' to be transferred as 'myfile.txt.tmp' and then renamed to 'myfile.txt' when the transfer is complete.

Alternatively the following options can be used to control the file transfer in more detail:

```
-et <extn>     transfer file with file extension <extn>     
-es <extn>     when transfer is done, change the extension of the SOURCE file to <extn>
-ed <extn>     when transfer is done, chnage the extension of the DESTINATION file to <extn>
```

All these options (including '-t') are also honored on 'get' commands, but beware that the sense of 'source' and 'destination', and thus of the '-es' and '-ed' are reversed, as the remote server is now the source, and the local one the destination.



FILE ENCRYPTION
===============

fileferry supports encrypting uploaded files, and decrypting them on download. This is done using one of these external programs: openssl, infozip, gpg or 7zip.



IMAGE SUPPORT
=============

fileferry supports displaying images with the `show` command. The `-img` or `-image` option specifies that the 'show' command should display the file as an image instead of just outputting it as a stream of characters to the screen. e.g.:

```
show -image *.jpg
```

this will launch an external image viewer if one can be found. The default list of image viewers is:

image magick 'display'
fim
feh
mage
xv
imlib2_view
gqview
qimageviewer
links -g

This list can be changed using 'set viewers'.

Alternatively fileferry can display images in terminals that have sixel support. Either the `-sixel` command-line option, which will set sixel support 'on' for the whole session, or on a per-command basis with the same option given to the 'show' command, like so:

```
show -image -sixel *.jpg
```

or you can turn on the 'sixel' option in settings, and 'show -image' will then default to sixel mode.

The sixel feature requires either the image-magick 'convert' command or 'img2sixel' to be available.


AVAILABLE FILESTORE COMMANDS
============================

```
help                 print this command list
help <command>       detailed help for <command>
exit                 quit program
quit                 quit program
set                  show settings
set <name> <value>   change setting '<name>' to '<value>'
cd <path>            change remote directory to <path>
chdir <path>         change remote directory to <path>
lcd <path>           change local directory to <path>
lchdir <path>        change local directory to <path>
pwd                  show current remote directory
lpwd                 show current local directory
ls <path>            list remote directory. <path> is optional
lls <path>           list local directory. <path> is optional
stats <path>         statistics for remote directory. <path> is optional
lstats <path>        statistics local directory. <path> is optional
mkdir <path>         make a remote directory <path>
lmkdir <path>        make a local directory <path>
rm <path>            delete remote file at <path> (wildcards allowed)
lrm <path>           delete local file at <path> (wildcards allowed)
del <path>           delete remote file at <path> (wildcards allowed)
ldel <path>          delete local file at <path> (wildcards allowed)
chmod <mode> <path>  change mode of files in <path> (wildcards allowed)
chmod <mode> <path>  change mode of files in <path> (wildcards allowed)
exists <path>        check if files matching <path> exist (wildcards allowed)
lexists <path>       check if files matching <path> exist (wildcards allowed)
get <path>           copy remote file at <path> to local (wildcards allowed)
put <path>           copy local file at <path> to remote (wildcards allowed)
share <path>         get 'sharing' url for remote file at <path> (if filestore supports this)
show <path>          output contents remote file at <path> to screen
lshow <path>         output contents local file at <path> to screen
cp <src> <dest>      copy remote file at <src> to remote <dest>
copy <src> <dest>    copy remote file at <src> to remote <dest>
lcp <src> <dest>     copy local file at <src> to local <dest>
lcopy <src> <dest>   copy local file at <src> to local <dest>
mv <src> <dest>      move/rename remote file at <src> to remote <dest>
move <src> <dest>    move/rename remote file at <src> to remote <dest>
rename <src> <dest>  move/rename remote file at <src> to remote <dest>
lmv <src> <dest>     move/rename local file at <src> to local <dest>
lmove <src> <dest>   move/rename local file at <src> to local <dest>
lrename <src> <dest> move/rename local file at <src> to local <dest>
chext <new extn> <files>  change file extention of remote files to <new extn>. (wildcards allowed)
lchext <new extn> <files>  change file extention of local files to <new extn>. (wildcards allowed)
info encrypt         print info about transport encryption on the current connection
info usage           print info about disk/quota usage
crc <path>           print checksum for file at <path> on remote host (only on services that support this)
lcrc <path>          print checksum for file at <path> on local host
md5 <path>           print md5 hash for file at <path> on remote host (only on services that support this)
lmd5 <path>          print md5 hash for file at <path> on local host
sha1 <path>          print sha1 hash for file at <path> on remote host (only on services that support this)
lsha1 <path>         print sha1 hash for file at <path> on local host
sha256 <path>        print sha256 hash for file at <path> on remote host (only on services that support this)
lsha256 <path>       print sha256 hash for file at <path> on local host
```
