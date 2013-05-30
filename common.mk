profiledir = $(localstatedir)/$(PACKAGE)/profiles

plugindir = $(pkglibdir)
pluginsodir = $(plugindir)/so
pluginexecdir = $(plugindir)/exec
pluginhaldir = $(plugindir)/hal
dbusdir = $(DBUS_SYS_DIR)

clean-local:
	rm -f *~
