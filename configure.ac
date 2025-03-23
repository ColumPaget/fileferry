AC_INIT([fileferry],[3.6])

AC_PROG_CC
AC_LANG([C])
AC_PROG_MAKE_SET
AC_PROG_EGREP

AC_SYS_LARGEFILE


AC_ARG_ENABLE(ftp, [  --enable-ftp		enable FTP/FTPS file transfer (default=yes) ], cf_enable_ftp=$enableval )
AC_ARG_ENABLE(sftp, [  --enable-sftp		enable SFTP file transfer (default=yes) ], cf_enable_sftp=$enableval )
AC_ARG_ENABLE(http, [  --enable-http		enable HTTP/HTTPS/WEBDAV file transfer (default=yes) ], cf_enable_http=$enableval )
AC_ARG_ENABLE(pop3, [  --enable-pop3		enable POP3/POP3S file transfer (default=yes) ], cf_enable_pop3=$enableval )
AC_ARG_ENABLE(syno, [  --enable-syno		enable SYNOLOGY NAS API file transfer (default=no) ], cf_enable_syno=$enableval )
AC_ARG_ENABLE(synology, [  --enable-synology		enable SYNOLOGY NAS API file transfer (default=no) ], cf_enable_syno=$enableval )
AC_ARG_ENABLE(faw, [  --enable-faw 		enable FilesAnywhere API file transfer (default=no) ], cf_enable_faw=$enableval )
AC_ARG_ENABLE(dropbox, [  --enable-dropbox 		enable DROPBOX API file transfer (default=no) ], cf_enable_dropbox=$enableval )
AC_ARG_ENABLE(gdrive, [  --enable-gdrive 		enable GOOGLEDRIVE API file transfer (default=no) ], cf_enable_gdrive=$enableval )
AC_ARG_ENABLE(gofile, [  --enable-gofile 		enable GOFILE API file transfer (default=no) ], cf_enable_gofile=$enableval )
AC_ARG_ENABLE(filebin, [  --enable-filebin		enable FILEBIN API file transfer (default=no) ], cf_enable_filebin=$enableval )
AC_ARG_ENABLE(allfs, [  --enable-allfs		enable all filestore types (default=no) ], cf_enable_allfs=$enableval )

AC_ARG_WITH(libuseful, [  --with-libuseful		use system-wide libUseful (default=auto) ], cf_system_libuseful=$withval )

dnl These libraries are required by libuseful. We only really need them if static linking, but
dnl let's be lazy and just include them all anyway
AC_CHECK_LIB(z,deflate,,)
AC_CHECK_LIB(cap,cap_init,,)
AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init
dnl New versions of openssl seem to lack EVP_CIPHER_CTX_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new,,)
AC_CHECK_LIB(ssl,SSL_library_init,,)
dnl New versions of openssl seem to lack SSL_library_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
AC_CHECK_LIB(ssl,OPENSSL_init_ssl,,)


if test "$cf_enable_allfs" == "yes"
then
cf_enable_ftp="yes"
cf_enable_sftp="yes"
cf_enable_http="yes"
cf_enable_syno="yes"
cf_enable_pop3="yes"
cf_enable_faw="yes"
cf_enable_dropbox="yes"
cf_enable_gdrive="yes"
cf_enable_filebin="yes"
fi

if test "$cf_enable_ftp" != "no"
then
AC_SUBST(FILESTORE_BUILTIN_FTP, "Y")
fi

if test "$cf_enable_sftp" != "no"
then
AC_SUBST(FILESTORE_BUILTIN_SFTP, "Y")
fi

if test "$cf_enable_http" != "no"
then
AC_SUBST(FILESTORE_BUILTIN_HTTP, "Y")
fi

if test "$cf_enable_syno" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_SYNO, "Y")
fi

if test "$cf_enable_faw" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_FAW, "Y")
fi

if test "$cf_enable_dropbox" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_DROPBOX, "Y")
fi

if test "$cf_enable_gdrive" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_GDRIVE, "Y")
fi

if test "$cf_enable_pop3" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_POP3, "Y")
fi

if test "$cf_enable_gofile" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_GOFILE, "Y")
fi

if test "$cf_enable_filebin" == "yes"
then
AC_SUBST(FILESTORE_BUILTIN_FILEBIN, "Y")
fi


if test "$cf_system_libuseful" != "no"
then
	AC_CHECK_LIB([Useful-5],GetToken,[cf_have_libUseful5=yes],[cf_have_libUseful5=no])
	AC_CHECK_LIB([Useful-5],TerminalReadPromptWithHistory,AC_DEFINE([HAVE_PROMPT_HISTORY],"Y"))

	if test "$cf_have_libUseful5" = "yes"
	then
	AC_CHECK_HEADERS([libUseful-5/libUseful.h])
	else
	AC_CHECK_LIB([Useful-4],GetToken,[cf_have_libUseful4=yes],[cf_have_libUseful4=no])
	AC_CHECK_HEADERS([libUseful-4/libUseful.h])
	fi
fi

if test "$cf_have_libUseful5" = "yes" 
then
  echo "installed libUseful-5 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-5],GetToken)
  AC_CHECK_LIB([Useful-5],AppendVar)

	if test "$ac_cv_lib_Useful_5_AppendVar" = "yes" 
	then
	AC_DEFINE(HAVE_APPEND_VAR)
	fi

elif test "$cf_have_libUseful4" = "yes" 
then
  echo "installed libUseful-4 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-4],GetToken)
else
  AC_DEFINE(USE_LIBUSEFUL_BUNDLED)
  AC_SUBST(LIBUSEFUL_BUNDLED,"libUseful-bundled/libUseful.a")
  AC_CONFIG_SUBDIRS([libUseful-bundled])
  AC_DEFINE(HAVE_APPEND_VAR)
  AC_DEFINE([HAVE_PROMPT_HISTORY],"Y")
fi


dnl read Makefile.in and write Makefile
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
AC_CONFIG_FILES([filestore_drivers/Makefile])
AC_OUTPUT

