DEFS =						\
	-DPREFIX=\"$(prefix)\"			\
	-DBINDIR=\"$(bindir)\"			\
	-DDATADIR=\"$(datadir)\"

common_AM_CPPFLAGS = 				\
	-I$(top_srcdir)/src			\
	$(GLIB_CFLAGS) 				\
	$(UPOWER_CFLAGS)			\
	$(DBUS_CFLAGS)

common_LDADD = 					\
	$(GLIB_LIBS)				\
	$(UPOWER_LIBS)				\
	$(DBUS_LIBS)
