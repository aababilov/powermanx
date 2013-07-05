include $(top_srcdir)/common.mk
include $(top_srcdir)/common.src.mk

EXTRA_DIST = servicename-infos.xml

BUILT_SOURCES = pm_svr_bindings.h

sbin_PROGRAMS = pmx-daemon

AM_CPPFLAGS =					\
	$(common_AM_CPPFLAGS)

pmx_daemon_LDADD = 			\
	$(common_LDADD)			\
	$(builddir)/../libpowermanx/libpowermanx.la

pmx_daemon_SOURCES =			\
	main.cpp			\
	xpm_manager.cpp			\
	xpm_manager.hpp			\
	pm_svr_bindings.h

pm_svr_bindings.h: servicename-infos.xml
	dbus-binding-tool --prefix=powerman --mode=glib-server $< > $@
