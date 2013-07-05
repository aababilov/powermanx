include $(top_srcdir)/common.mk
include $(top_srcdir)/common.src.mk

AM_CPPFLAGS = 					\
	$(common_AM_CPPFLAGS)			\
	-DPLUGINEXECDIR=\"$(pluginexecdir)\"	\
	-DPLUGINHALDIR=\"$(pluginhaldir)\"	\
	-DPLUGINSODIR=\"$(pluginsodir)\"	\
	-DPROFILEDIR=\"$(profiledir)\"

lib_LTLIBRARIES = libpowermanx.la

libpowermanx_la_SOURCES = 	\
	signal_slot.hpp 	\
	signal_slot.cpp		\
	profile.hpp		\
	profile.cpp		\
	slot_exe.hpp		\
	slot_exe.cpp		\
	variant.hpp		\
	utils.hpp		\
	utils.cpp		\
	main.cpp		\
	libpowermanx.hpp

libpowermanx_la_LIBADD =	\
	$(common_LDADD)
