include $(top_srcdir)/common.mk
include $(top_srcdir)/common.src.mk

pluginso_LTLIBRARIES = baseplugins.la

INCLUDES = $(common_INCLUDES)		\
        $(XSCRNSAVER_CFLAGS)

baseplugins_la_SOURCES = 		\
	baseplugins.hpp 		\
	baseplugins.cpp			\
	get_idletime.hpp 		\
	get_idletime.cpp

baseplugins_la_LDFLAGS =		\
	-avoid-version			\
	-module				\
	-no-undefined

baseplugins_la_LIBADD = 		\
	$(common_LDADD)			\
        $(XSCRNSAVER_LIBS)		\
	$(top_builddir)/src/libpowermanx/libpowermanx.la