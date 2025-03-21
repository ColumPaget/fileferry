FLAGS=-g -O2 -DPACKAGE_NAME=\"fileferry\" -DPACKAGE_TARNAME=\"fileferry\" -DPACKAGE_VERSION=\"3.5\" -DPACKAGE_STRING=\"fileferry\ 3.5\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -D_FILE_OFFSET_BITS=64 -DHAVE_LIBZ=1 -DHAVE_LIBCAP=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBSSL=1 -DHAVE_LIBSSL=1 -DHAVE_PROMPT_HISTORY=\"Y\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_APPEND_VAR=1
PREFIX=/usr/local
LIBS=-lUseful-5 -lUseful-5 -lssl -lssl -lcrypto -lcrypto -lcap -lz 
STATIC_LIBUSEFUL=
FILESTORE_BUILTIN_FTP=Y
FILESTORE_OBJ=filestore_drivers.o inet_protocols.o ls_decode.o localdisk.o
FILESTORE_BUILTIN_FTP=Y
FILESTORE_BUILTIN_SFTP=Y
FILESTORE_BUILTIN_HTTP=Y
FILESTORE_BUILTIN_POP3=
FILESTORE_BUILTIN_GDRIVE=
FILESTORE_BUILTIN_DROPBOX=
FILESTORE_BUILTIN_FAW=
FILESTORE_BUILTIN_GOFILE=
FILESTORE_BUILTIN_FILEBIN=
FILESTORE_BUILTIN_SYNO=


ifeq ($(FILESTORE_BUILTIN_FTP),Y)
FILESTORE_OBJ += ftp.o
FLAGS += "-DFILESTORE_BUILTIN_FTP"
endif

ifeq ($(FILESTORE_BUILTIN_SFTP),Y)
FILESTORE_OBJ += sftp.o
FLAGS += "-DFILESTORE_BUILTIN_SFTP"
endif

ifeq ($(FILESTORE_BUILTIN_HTTP),Y)
FILESTORE_OBJ += http.o webdav.o
FLAGS += "-DFILESTORE_BUILTIN_HTTP"
endif

ifeq ($(FILESTORE_BUILTIN_POP3),Y)
FILESTORE_OBJ += pop3.o
FLAGS += "-DFILESTORE_BUILTIN_POP3"
endif

ifeq ($(FILESTORE_BUILTIN_GDRIVE),Y)
FILESTORE_OBJ += gdrive.o
FLAGS += "-DFILESTORE_BUILTIN_GDRIVE"
endif

ifeq ($(FILESTORE_BUILTIN_DROPBOX),Y)
FILESTORE_OBJ += dropbox.o
FLAGS += "-DFILESTORE_BUILTIN_DROPBOX"
endif

ifeq ($(FILESTORE_BUILTIN_FAW),Y)
FILESTORE_OBJ += filesanywhere.o
FLAGS += "-DFILESTORE_BUILTIN_FAW"
endif

ifeq ($(FILESTORE_BUILTIN_GOFILE),Y)
FILESTORE_OBJ += gofile.o
FLAGS += "-DFILESTORE_BUILTIN_GOFILE"
endif

ifeq ($(FILESTORE_BUILTIN_FILEBIN),Y)
FILESTORE_OBJ += filebin.o
FLAGS += "-DFILESTORE_BUILTIN_FILEBIN"
endif

ifeq ($(FILESTORE_BUILTIN_SYNO),Y)
FILESTORE_OBJ += synology.o
FLAGS += "-DFILESTORE_BUILTIN_SYNO"
endif



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
