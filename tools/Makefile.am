
bin_PROGRAMS = genpkglist gensrclist
bin_SCRIPTS = genbasedir

EXTRA_DIST = genbasedir

LDADD = ../apt-pkg/libapt-pkg.la $(RPMLIBS)

genpkglist_SOURCES = genpkglist.cc cached_md5.cc cached_md5.h
gensrclist_SOURCES = gensrclist.cc cached_md5.cc cached_md5.h
