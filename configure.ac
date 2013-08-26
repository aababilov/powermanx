AC_PREREQ([2.69])
AC_INIT([powermanx], [0.2])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

if test "$GCC" = "yes"; then
    CPPFLAGS="$CPPFLAGS -Wcast-align -Wno-uninitialized -Wall -Wformat-security"
fi

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/time.h unistd.h utmpx.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_FUNC_FORK
AC_HEADER_MAJOR
AC_CHECK_FUNCS([atexit dup2 gettimeofday putenv strcasecmp strchr strerror strrchr strstr strtoul])

dnl ---------------------------------------------------------------------------
dnl - libtool
dnl ---------------------------------------------------------------------------
LT_PREREQ([2.2.6])
LT_INIT

dnl ---------------------------------------------------------------------------
dnl - i18n
dnl ---------------------------------------------------------------------------
IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=powermanx
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])
AM_GLIB_GNU_GETTEXT

dnl ---------------------------------------------------------------------------
dnl - Library dependencies
dnl ---------------------------------------------------------------------------
GLIB_REQUIRED=2.24
GTK_REQUIRED=3.1
DBUS_REQUIRED=1.5
DBUS_GLIB_REQUIRED=0.70
UPOWER_REQUIRED=0.9
NOTIFY_REQUIRED=0.7

dnl ---------------------------------------------------------------------------
dnl - Make above strings available for packaging files (e.g. rpm spec files)
dnl ---------------------------------------------------------------------------
AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)
AC_SUBST(DBUS_REQUIRED)
AC_SUBST(DBUS_GLIB_REQUIRED)
AC_SUBST(UPOWER_REQUIRED)
AC_SUBST(NOTIFY_REQUIRED)

dnl ---------------------------------------------------------------------------
dnl - Check library dependencies
dnl ---------------------------------------------------------------------------
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED gobject-2.0 gmodule-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(DBUS,[
 dbus-1 >= $DBUS_REQUIRED
 dbus-glib-1 >= $DBUS_GLIB_REQUIRED
 gthread-2.0])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(GUDEV, gudev-1.0)
AC_SUBST(GUDEV_CFLAGS)
AC_SUBST(GUDEV_LIBS)

PKG_CHECK_MODULES(UPOWER, upower-glib >= $UPOWER_REQUIRED)
AC_SUBST(UPOWER_CFLAGS)
AC_SUBST(UPOWER_LIBS)

PKG_CHECK_MODULES(LIGHTDM, liblightdm-gobject-1)
AC_SUBST(LIGHTDM_CFLAGS)
AC_SUBST(LIGHTDM_LIBS)

PKG_CHECK_MODULES(NOTIFY, [libnotify >= $NOTIFY_REQUIRED])
AC_SUBST(NOTIFY_CFLAGS)
AC_SUBST(NOTIFY_LIBS)

PKG_CHECK_MODULES(XSCRNSAVER, xscrnsaver x11 xext)
AC_SUBST(XSCRNSAVER_CFLAGS)
AC_SUBST(XSCRNSAVER_LIBS)

AC_SUBST(JSONCPP_CFLAGS, -I/usr/include/jsoncpp)
AC_SUBST(JSONCPP_LIBS, -ljsoncpp)

dnl ---------------------------------------------------------------------------
dnl - Check for systemd
dnl ---------------------------------------------------------------------------
AC_ARG_WITH([systemdsystemunitdir],
            AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
            [],
            [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
AM_CONDITIONAL(HAVE_SYSTEMDSYSTEMUNITDIR, [test -n "$with_systemdsystemunitdir"])

dnl ---------------------------------------------------------------------------
dnl - Generate files
dnl ---------------------------------------------------------------------------
AC_CONFIG_FILES([Makefile
                 data/Makefile
                 data/icons/16x16/Makefile
                 data/icons/16x16/apps/Makefile
                 data/icons/16x16/status/Makefile
                 data/icons/22x22/Makefile
                 data/icons/22x22/apps/Makefile
                 data/icons/22x22/status/Makefile
                 data/icons/24x24/Makefile
                 data/icons/24x24/apps/Makefile
                 data/icons/24x24/status/Makefile
                 data/icons/48x48/Makefile
                 data/icons/48x48/apps/Makefile
                 data/icons/48x48/status/Makefile
                 data/icons/Makefile
                 data/icons/scalable/Makefile
                 data/icons/scalable/apps/Makefile
                 data/icons/scalable/status/Makefile
                 data/profiles/Makefile
                 doc/Makefile
                 plugins/Makefile
                 plugins/exec/Makefile
                 plugins/so/Makefile
                 plugins/so/baseplugins/Makefile
                 po/Makefile.in
                 src/Makefile
                 src/libpowermanx/Makefile
                 src/pmx-applet/Makefile
                 src/pmx-daemon/Makefile
])
AC_OUTPUT
