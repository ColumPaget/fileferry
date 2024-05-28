AC_INIT([fileferry], [2.0])

AC_PROG_CC
AC_LANG_C
AC_PROG_MAKE_SET
AC_HEADER_STDC
AC_SYS_LARGEFILE


AC_ARG_WITH(libuseful, [  --with-libuseful		use system-wide libUseful (default=auto) ], cf_system_libuseful=$withval )

dnl These libraries are required by libuseful. We only really need them if static linking, but
dnl let's be lazy and just include them all anyway
AC_CHECK_LIB(z,deflate,,)
AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
AC_CHECK_LIB(ssl,SSL_library_init,,)
AC_CHECK_LIB(cap,cap_init,,)



if test "$cf_system_libuseful" != "no"
then
	AC_CHECK_LIB([Useful-5],GetToken,[cf_have_libUseful5=yes],[cf_have_libUseful5=no])
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
elif test "$cf_have_libUseful4" = "yes" 
then
  echo "installed libUseful-4 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-4],GetToken)
else
  AC_SUBST(LIBUSEFUL_BUNDLED,"libUseful-5/libUseful.a")
  AC_CONFIG_SUBDIRS([libUseful-5])
fi

dnl read Makefile.in and write Makefile
AC_OUTPUT(Makefile)
AC_OUTPUT([filestore_drivers/Makefile])

