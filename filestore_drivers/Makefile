FILESTORE_OBJ=filestore_drivers.o inet_protocols.o ls_decode.o localdisk.o ftp.o sftp.o http.o webdav.o pop3.o gdrive.o dropbox.o filesanywhere.o gofile.o filebin.o synology.o
BUILTIN=-DFILESTORE_BUILTIN_SFTP -DFILESTORE_BUILTIN_HTTP -DFILESTORE_BUILTIN_FTP -DFILESTORE_BUILTIN_POP3 -DFILESTORE_BUILTIN_GDRIVE -DFILESTORE_BUILTIN_DROPBOX -DFILESTORE_BUILTIN_GOFILE -DFILESTORE_BUILTIN_FILEBIN -DFILESTORE_BUILTIN_FILESANYWHERE -DFILESTORE_BUILTIN_SYNOLOGY
FLAGS=-g -O2 -DPACKAGE_NAME=\"fileferry\" -DPACKAGE_TARNAME=\"fileferry\" -DPACKAGE_VERSION=\"3.0\" -DPACKAGE_STRING=\"fileferry\ 3.0\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL_5=1 $(BUILTIN) 
PREFIX=/usr/local
LIBS=-lUseful-5 
STATIC_LIBUSEFUL=

fileferry_builtin_drivers.a: $(FILESTORE_OBJ)
	ar rcs fileferry_builtin_drivers.a $(FILESTORE_OBJ)

filestore_drivers.o: filestore_drivers.h filestore_drivers.c
	gcc $(FLAGS) -c filestore_drivers.c

ls_decode.o: ls_decode.h ls_decode.c
	gcc $(FLAGS) -c ls_decode.c

inet_protocols.o: inet_protocols.h inet_protocols.c
	gcc $(FLAGS) -c inet_protocols.c

localdisk.o: localdisk.h localdisk.c
	gcc $(FLAGS) -c localdisk.c

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


clean:
	rm *.o *.a *.orig 
