
bin_PROGRAMS = apt-get apt-cache apt-cdrom apt-config

EXTRA_DIST = indexcopy.cc indexcopy.h

if COMPILE_APTSHELL
bin_PROGRAMS += apt-shell
endif

LDADD = ../apt-pkg/libapt-pkg.la $(RPMLIBS)

apt_cache_SOURCES = apt-cache.cc
apt_get_SOURCES = apt-get.cc acqprogress.cc acqprogress.h
apt_shell_SOURCES = apt-shell.cc acqprogress.cc acqprogress.h
apt_shell_LDADD = $(LDADD) -lreadline -lncurses
apt_config_SOURCES = apt-config.cc
apt_cdrom_SOURCES = apt-cdrom.cc rpmindexcopy.cc rpmindexcopy.h
