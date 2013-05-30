DEFS =						\
	-DPREFIX=\"$(prefix)\"			\
	-DBINDIR=\"$(bindir)\"			\
	-DDATADIR=\"$(datadir)\"

common_INCLUDES = 				\
	-I$(top_srcdir)/src			\
	$(GLIB_CFLAGS) 				\
	$(HAL_CFLAGS)				\
	$(DBUS_CFLAGS)

common_LDADD = 					\
	$(GLIB_LIBS)				\
	$(HAL_LIBS)				\
	$(DBUS_LIBS)
