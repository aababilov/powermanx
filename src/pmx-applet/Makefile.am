include $(top_srcdir)/common.mk
include $(top_srcdir)/common.src.mk

bin_PROGRAMS = pmx-applet

AM_CPPFLAGS =				\
	$(common_AM_CPPFLAGS)		\
	-DPKGDATADIR=\"$(pkgdatadir)\"	\
	$(NOTIFY_CFLAGS)                \
	$(GTK_CFLAGS)

pmx_applet_LDADD = 			\
	$(common_LDADD) 		\
	$(GTK_LIBS)			\
	$(NOTIFY_LIBS)                  \
	$(builddir)/../libpowermanx/libpowermanx.la

pmx_applet_SOURCES =		\
	main.cpp		\
	status_icon.hpp		\
	status_icon.cpp		\
	profile_edit.hpp	\
	profile_edit.cpp	\
	app_utils.hpp
