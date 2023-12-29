OBJ=common.o settings.o proc.o encrypt.o password.o file_transfer.o filestore.o filestore_dirlist.o filestore_drivers.o filestore_index.o saved_filestores.o stdout.o fileitem.o file_include_exclude.o list_content_type.o ls_decode.o html.o rss.o webdav.o localdisk.o inet_protocols.o ftp.o sftp.o http.o pop3.o gdrive.o dropbox.o filesanywhere.o gofile.o filebin.o filecache.o image_display.o commands.o ui.o help.o errors_and_logging.o
BUILTIN=-DFILESTORE_BUILTIN_SFTP -DFILESTORE_BUILTIN_HTTP -DFILESTORE_BUILTIN_FTP -DFILESTORE_BUILTIN_POP3 -DFILESTORE_BUILTIN_GDRIVE -DFILESTORE_BUILTIN_DROPBOX -DFILESTORE_BUILTIN_GOFILE -DFILESTORE_BUILTIN_FILEBIN -DFILESTORE_BUILTIN_FILESANYWHERE
FLAGS=-g -D_FILE_OFFSET_BITS=64 -DPACKAGE_VERSION=2.0 $(BUILTIN) 
PREFIX=/usr/local
LIBS=-lcap -lssl -lcrypto -lz 
STATIC_LIBUSEFUL=libUseful-5/libUseful.a

all: $(OBJ) main.c $(STATIC_LIBUSEFUL)
	gcc $(FLAGS) -ofileferry $(OBJ) main.c $(STATIC_LIBUSEFUL) $(LIBS)

libUseful-5/libUseful.a:
	make -C libUseful-5

common.o: common.h common.c
	gcc $(FLAGS) -c common.c

proc.o: proc.h proc.c
	gcc $(FLAGS) -c proc.c

settings.o: settings.h settings.c
	gcc $(FLAGS) -c settings.c

fileitem.o: fileitem.h fileitem.c
	gcc $(FLAGS) -c fileitem.c

file_transfer.o: file_transfer.h file_transfer.c
	gcc $(FLAGS) -c file_transfer.c

ui.o: ui.h ui.c
	gcc $(FLAGS) -c ui.c

stdout.o: stdout.h stdout.c
	gcc $(FLAGS) -c stdout.c

help.o: help.h help.c
	gcc $(FLAGS) -c help.c

list_content_type.o: list_content_type.h list_content_type.c
	gcc $(FLAGS) -c list_content_type.c

html.o: html.h html.c
	gcc $(FLAGS) -c html.c

rss.o: rss.h rss.c
	gcc $(FLAGS) -c rss.c

saved_filestores.o: saved_filestores.h saved_filestores.c
	gcc $(FLAGS) -c saved_filestores.c

filestore.o: filestore.h filestore.c
	gcc $(FLAGS) -c filestore.c

filestore_dirlist.o: filestore_dirlist.h filestore_dirlist.c filestore.h
	gcc $(FLAGS) -c filestore_dirlist.c


filestore_drivers.o: filestore_drivers.h filestore_drivers.c
	gcc $(FLAGS) -c filestore_drivers.c

filestore_index.o: filestore_index.h filestore_index.c
	gcc $(FLAGS) -c filestore_index.c

file_include_exclude.o: file_include_exclude.h file_include_exclude.c
	gcc $(FLAGS) -c file_include_exclude.c

localdisk.o: localdisk.h localdisk.c
	gcc $(FLAGS) -c localdisk.c

ls_decode.o: ls_decode.h ls_decode.c
	gcc $(FLAGS) -c ls_decode.c

commands.o: commands.h commands.c
	gcc $(FLAGS) -c commands.c

filecache.o: filecache.h filecache.c
	gcc $(FLAGS) -c filecache.c

inet_protocols.o: inet_protocols.h inet_protocols.c
	gcc $(FLAGS) -c inet_protocols.c

sftp.o: sftp.h sftp.c
	gcc $(FLAGS) -c sftp.c

ftp.o: ftp.h ftp.c
	gcc $(FLAGS) -c ftp.c

pop3.o: pop3.h pop3.c
	gcc $(FLAGS) -c pop3.c

gdrive.o: gdrive.h gdrive.c
	gcc $(FLAGS) -c gdrive.c

dropbox.o: dropbox.h dropbox.c
	gcc $(FLAGS) -c dropbox.c

filesanywhere.o: filesanywhere.h filesanywhere.c
	gcc $(FLAGS) -c filesanywhere.c

gofile.o: gofile.h gofile.c
	gcc $(FLAGS) -c gofile.c

filebin.o: filebin.h filebin.c
	gcc $(FLAGS) -c filebin.c

http.o: http.h http.c
	gcc $(FLAGS) -c http.c

webdav.o: webdav.h webdav.c
	gcc $(FLAGS) -c webdav.c

image_display.o: image_display.h image_display.c
	gcc $(FLAGS) -c image_display.c

encrypt.o: encrypt.h encrypt.c
	gcc $(FLAGS) -c encrypt.c

password.o: password.h password.c
	gcc $(FLAGS) -c password.c

errors_and_logging.o: errors_and_logging.h errors_and_logging.c
	gcc $(FLAGS) -c errors_and_logging.c


install:
	cp fileferry $(PREFIX)/bin

clean:
	rm *.o */*.o */*.a */*.so *.orig fileferry
