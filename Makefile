OBJ=common.o settings.o proc.o encrypt.o password.o file_transfer.o filestore.o filestore_dirlist.o filestore_index.o saved_filestores.o stdout.o fileitem.o file_include_exclude.o list_content_type.o html.o rss.o filecache.o image_display.o extra_hashes.o commands.o ui.o help.o errors_and_logging.o filestore_drivers/fileferry_builtin_drivers.a
FLAGS=-g -D_FILE_OFFSET_BITS=64 -DPACKAGE_NAME=\"fileferry\" -DPACKAGE_TARNAME=\"fileferry\" -DPACKAGE_VERSION=\"3.6\" -DPACKAGE_STRING=\"fileferry\ 3.6\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -D_FILE_OFFSET_BITS=64 -DHAVE_LIBZ=1 -DHAVE_LIBCAP=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBSSL=1 -DHAVE_LIBSSL=1 -DHAVE_PROMPT_HISTORY=\"Y\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_APPEND_VAR=1
PREFIX=/usr/local
LIBS=-lUseful-5 -lUseful-5 -lssl -lssl -lcrypto -lcrypto -lcap -lz 
STATIC_LIBUSEFUL=


all: $(OBJ) main.c $(STATIC_LIBUSEFUL)
	gcc $(FLAGS) -o fileferry $(OBJ) main.c $(STATIC_LIBUSEFUL) $(LIBS)

libUseful-bundled/libUseful.a:
	make -C libUseful-bundled

filestore_drivers/fileferry_builtin_drivers.a: $(wildcard filestore_drivers/*.c)
	make -C filestore_drivers

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

image_display.o: image_display.h image_display.c
	gcc $(FLAGS) -c image_display.c

encrypt.o: encrypt.h encrypt.c
	gcc $(FLAGS) -c encrypt.c

extra_hashes.o: extra_hashes.h extra_hashes.c
	gcc $(FLAGS) -c extra_hashes.c

password.o: password.h password.c
	gcc $(FLAGS) -c password.c

errors_and_logging.o: errors_and_logging.h errors_and_logging.c
	gcc $(FLAGS) -c errors_and_logging.c

sftp.o: sftp.h sftp.c
	gcc $(FLAGS) -c sftp.c

ftp.o: ftp.h ftp.c
	gcc $(FLAGS) -c ftp.c

http.o: http.h http.c
	gcc $(FLAGS) -c http.c

webdav.o: webdav.h webdav.c
	gcc $(FLAGS) -c webdav.c

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

synology.o: synology.h synology.c
	gcc $(FLAGS) -c synology.c


install:
	cp fileferry $(PREFIX)/bin

clean:
	rm *.orig */*.orig *.o */*.o */*.a */*.so */*.orig fileferry
