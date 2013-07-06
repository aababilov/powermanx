profiledir = $(localstatedir)/$(PACKAGE)/profiles

plugindir = $(pkglibdir)
pluginsodir = $(plugindir)/so
pluginexecdir = $(plugindir)/exec
pluginhaldir = $(plugindir)/hal


MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in


clean-local:
	rm -f *~
