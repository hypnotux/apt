
all-local: copy-headers

EXTRA_DIST = \
	     archtable \
	     ostable \
	     sizetable \
	     config.h.in \
	     gettext.h \
	     inttypes.h \
	     statvfs.h \
	     netdb.h

COPYHEADERS = copy-apti18n

if NEED_INTTYPES_H
COPYHEADERS += copy-inttypes
endif
if NEED_STATVFS_H
COPYHEADERS += copy-statvfs
endif
if NEED_NETDB_H
COPYHEADERS += copy-netdb
endif

copy-headers: $(COPYHEADERS)

copy-apti18n:
	@$(mkinstalldirs) $(top_builddir)/include
	@echo "Copying gettext.h to $(top_builddir)/include/apti18n.h"
	@cp -f -p gettext.h $(top_builddir)/include/apti18n.h
	
copy-inttypes:
	@echo "Copying inttypes.h to $(top_builddir)/include/apti18n.h"
	@cp -f -p inttypes.h.in $(top_builddir)/include/inttypes.h

copy-statvfs:
	@echo "Copying statvfs.h to $(top_builddir)/include/apti18n.h"
	@cp -f -p statvfs.h $(top_builddir)/include/statvfs.h

copy-netdb:
	@echo "Copying netdb.h to $(top_builddir)/include/netdb.h"
	@cp -f -p netdb.h $(top_builddir)/include/netdb.h
