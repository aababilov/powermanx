AC_INIT([powermanx], [0.2])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_DISABLE_STATIC

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AM_CONFIG_HEADER(config.h)

if test "$GCC" = "yes"; then
    CPPFLAGS="$CPPFLAGS -Wcast-align -Wno-uninitialized -Wall -Wformat-security"
fi

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

PKG_CHECK_MODULES(UPOWER, upower-glib >= $UPOWER_REQUIRED)
AC_SUBST(UPOWER_CFLAGS)
AC_SUBST(UPOWER_LIBS)

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


AC_OUTPUT([Makefile
	src/Makefile
	src/pmx-applet/Makefile
	src/pmx-daemon/Makefile
	src/libpowermanx/Makefile
	profiles/Makefile
	plugins/Makefile
	plugins/exec/Makefile
	plugins/hal/Makefile
	plugins/so/Makefile
	plugins/so/baseplugins/Makefile
	data/Makefile
	data/icons/Makefile
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
	data/icons/scalable/Makefile
	data/icons/scalable/apps/Makefile
	data/icons/scalable/status/Makefile
	doc/Makefile
	])
