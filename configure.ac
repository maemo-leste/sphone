AC_INIT(sphone, 0.06)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC




dnl ***************************************************************************
dnl Internationalisation
dnl ***************************************************************************
GETTEXT_PACKAGE=sphone
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])



AM_PROG_LIBTOOL



PKG_CHECK_MODULES(SPHONE, [gtk+-2.0 >= 2.8 dbus-glib-1 unique-1.0 sqlite3 gstreamer-1.0 alsa])


AC_ARG_ENABLE(libprofile,
    [AS_HELP_STRING([--enable-libprofile],
             [Use libprofile for settings [default=yes]]
             )],
     [],
     [enable_libprofile="yes"])

AC_ARG_ENABLE(libhildon,
    [AS_HELP_STRING([--enable-libhildon],
             [Use libhildon for ui [default=yes]]
             )],
     [],
     [enable_libhildon="yes"])

if test "x$enable_libprofile" = "xyes"; then
    PKG_CHECK_MODULES(LIBPROFILE, profile)
    AC_SUBST(LIBPROFILE_CFLAGS)
    AC_SUBST(LIBPROFILE_LIBS)
    AC_DEFINE(ENABLE_LIBPROFILE, 1, [libprofile enable define])
fi

if test "x$enable_libhildon" = "xyes"; then
    PKG_CHECK_MODULES(LIBHILDON, hildon-1)
    AC_SUBST(LIBHILDON_CFLAGS)
    AC_SUBST(LIBHILDON_LIBS)
    AC_DEFINE(ENABLE_LIBHILDON, 1, [libhildon enable define])
fi

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
icons/Makefile
desktop/Makefile
])
