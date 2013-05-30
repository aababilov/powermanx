include $(top_srcdir)/common.mk
include $(top_srcdir)/common.src.mk

EXTRA_DIST = servicename-infos.xml

BUILT_SOURCES = pm_svr_bindings.h

sbin_PROGRAMS = pmx-daemon

INCLUDES =					\
	$(common_INCLUDES)			\
	-DPMXD_PID_FILE=\""$(PMXD_PID_FILE)"\"

pmx_daemon_LDADD = 			\
	$(common_LDADD)			\
	$(builddir)/../libpowermanx/libpowermanx.la

pmx_daemon_SOURCES =			\
	main.cpp			\
	pm_svr_bindings.h

pm_svr_bindings.h: servicename-infos.xml
	dbus-binding-tool --prefix=powerman --mode=glib-server $< > $@
