# -*- make -*-
BASE=..
SUBDIR=cmdline

# Bring in the default rules
include ../buildlib/defaults.mak

# CNC:2003-02-17 - Included $(RPMLIBS) in SLIBS for each program. We need
# 		   that in case the program is going to be linked with the
# 		   static libapt-pkg library.

# The apt-cache program
PROGRAM=apt-cache
SLIBS = -lapt-pkg $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = apt-cache.cc
include $(PROGRAM_H)

# The apt-get program
PROGRAM=apt-get
SLIBS = -lapt-pkg $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = apt-get.cc acqprogress.cc
include $(PROGRAM_H)

# CNC:2003-03-05
ifdef COMPILE_APTSHELL
# The apt-shell program
PROGRAM=apt-shell
SLIBS = -lapt-pkg -lreadline -lncurses $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = apt-shell.cc acqprogress.cc
include $(PROGRAM_H)
endif

# The apt-config program
PROGRAM=apt-config
SLIBS = -lapt-pkg $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = apt-config.cc
include $(PROGRAM_H)

# The apt-cdrom program
PROGRAM=apt-cdrom
SLIBS = -lapt-pkg $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
# CNC:2002-07-11
SOURCE = apt-cdrom.cc rpmindexcopy.cc
include $(PROGRAM_H)

# CNC:2003-02-15
WITH_APT_SORTPKGS=no
ifeq ($(WITH_APT_SORTPKGS),yes)
# The apt-sortpkgs program
PROGRAM=apt-sortpkgs
SLIBS = -lapt-pkg $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = apt-sortpkgs.cc
include $(PROGRAM_H)
endif

# CNC:2003-02-15
WITH_APT_EXTRACTTEMPLATES=no
ifeq ($(WITH_APT_EXTRACTTEMPLATES),yes)
# The apt-extracttemplates program
PROGRAM=apt-extracttemplates
SLIBS = -lapt-pkg -lapt-inst $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = apt-extracttemplates.cc 
include $(PROGRAM_H)
endif
