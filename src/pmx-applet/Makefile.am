include $(top_srcdir)/common.mk
include $(top_srcdir)/common.src.mk

bin_PROGRAMS = pmx-applet

INCLUDES =				\
	$(common_INCLUDES)		\
	-DPKGDATADIR=\"$(pkgdatadir)\"	\
	$(GTK_CFLAGS)

pmx_applet_LDADD = 			\
	$(common_LDADD) 		\
	$(GTK_LIBS)			\
	$(builddir)/../libpowermanx/libpowermanx.la

pmx_applet_SOURCES =		\
	main.cpp		\
	status_icon.hpp		\
	status_icon.cpp		\
	profile_edit.hpp	\
	profile_edit.cpp	\
	app_utils.hpp