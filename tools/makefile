
BASE=..
SUBDIR=tools

# Bring in the default rules
include ../buildlib/defaults.mak

PROGRAM=genpkglist
SLIBS = -lapt-pkg $(RPMLIBS)
LIB_MAKES = apt-pkg/makefile
SOURCE = genpkglist.cc cached_md5.cc
include $(PROGRAM_H)

PROGRAM=gensrclist
SLIBS = -lapt-pkg $(RPMLIBS) 
LIB_MAKES = apt-pkg/makefile
SOURCE = gensrclist.cc cached_md5.cc
include $(PROGRAM_H)

PROGRAM=hdlist2pkglist
SLIBS = -lapt-pkg $(RPMLIBS) 
LIB_MAKES = apt-pkg/makefile
SOURCE = hdlist2pkglist.cc
include $(PROGRAM_H)

#PROGRAM=apt-listconfigs
#SLIBS = -lapt-pkg $(RPMLIBS) 
#LIB_MAKES = apt-pkg/makefile
#SOURCE = apt-listconfigs.cc
#include $(PROGRAM_H)


