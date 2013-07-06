include $(top_srcdir)/common.mk

SUBDIRS = icons

dist_pkgdata_DATA = profile_edit.ui

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dist_dbusconf_DATA = powermanx.conf


edit = sed \
	-e 's|@sbindir[@]|$(sbindir)|g'


servicedir = $(datadir)/dbus-1/system-services
service_in_files = org.freedesktop.PowerManX.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(edit) $< >$@


systemdservice_in_files = powermanx.service.in

if HAVE_SYSTEMDSYSTEMUNITDIR
systemdservicedir       = $(systemdsystemunitdir)
systemdservice_DATA     = $(systemdservice_in_files:.service.in=.service)

$(systemdservice_DATA): $(systemdservice_in_files) Makefile
	$(edit) $< > $@
endif


EXTRA_DIST = $(service_in_files) $(systemdservice_in_files)

CLEANFILES =                                            \
        $(service_DATA)                                 \
        $(systemdservice_DATA)
