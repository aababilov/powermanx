profiledir = $(localstatedir)/lib/$(PACKAGE)/profiles

plugindir = $(pkglibdir)
pluginsodir = $(plugindir)/so
pluginexecdir = $(plugindir)/exec
pluginhaldir = $(plugindir)/hal

NULL =

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in


clean-local:
	rm -f *~
