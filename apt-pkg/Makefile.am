
lib_LTLIBRARIES = libapt-pkg.la

libapt_pkg_la_LIBADD = @RPMLIBS@
libapt_pkg_la_LDFLAGS = -version-info 0:0:0

libapt_pkg_la_SOURCES = \
	contrib/cdromutl.cc \
	contrib/cdromutl.h \
	contrib/cmndline.cc \
	contrib/cmndline.h \
	contrib/configuration.cc \
	contrib/configuration.h \
	contrib/crc-16.cc \
	contrib/crc-16.h \
	contrib/error.cc \
	contrib/error.h \
	contrib/fileutl.cc \
	contrib/fileutl.h \
	contrib/hashes.cc \
	contrib/hashes.h \
	contrib/md5.cc \
	contrib/md5.h \
	contrib/mmap.cc \
	contrib/mmap.h \
	contrib/progress.cc \
	contrib/progress.h \
	contrib/sha1.cc \
	contrib/sha1.h \
	contrib/sptr.h \
	contrib/strutl.cc \
	contrib/strutl.h \
	contrib/system.h \
	rpm/rpmhandler.cc \
	rpm/rpmhandler.h \
	rpm/rpmindexfile.cc \
	rpm/rpmindexfile.h \
	rpm/rpmlistparser.cc \
	rpm/rpmlistparser.h \
	rpm/rpmmisc.h \
	rpm/rpmpackagedata.cc \
	rpm/rpmpackagedata.h \
	rpm/rpmpm.cc \
	rpm/rpmpm.h \
	rpm/rpmrecords.cc \
	rpm/rpmrecords.h \
	rpm/rpmsrcrecords.cc \
	rpm/rpmsrcrecords.h \
	rpm/rpmsystem.cc \
	rpm/rpmsystem.h \
	rpm/rpmversion.cc \
	rpm/rpmversion.h \
	acquire.cc \
	acquire.h \
	acquire-item.cc \
	acquire-item.h \
	acquire-method.cc \
	acquire-method.h \
	acquire-worker.cc \
	acquire-worker.h \
	algorithms.cc \
	algorithms.h \
	cachefile.cc \
	cachefile.h \
	cacheiterators.h \
	clean.cc \
	clean.h \
	depcache.cc \
	depcache.h \
	indexfile.cc \
	indexfile.h \
	init.cc \
	init.h \
	orderlist.cc \
	orderlist.h \
	packagemanager.cc \
	packagemanager.h \
	pkgcache.cc \
	pkgcachegen.cc \
	pkgcachegen.h \
	pkgcache.h \
	pkgrecords.cc \
	pkgrecords.h \
	pkgsystem.cc \
	pkgsystem.h \
	policy.cc \
	policy.h \
	repository.cc \
	repository.h \
	sourcelist.cc \
	sourcelist.h \
	srcrecords.cc \
	srcrecords.h \
	tagfile.cc \
	tagfile.h \
	version.cc \
	version.h \
	versionmatch.cc \
	versionmatch.h

PRIVATEHEADERS = contrib/system.h

includedir=${prefix}/include/apt-pkg
include_HEADERS = $(filter-out \
		    $(PRIVATEHEADERS), \
		    $(filter %.h,$(libapt_pkg_la_SOURCES)))

# This is a hack to allow us to maintain the original
# APT system of a a flat apt-pkg/ header directory.
PRIVREALHEADERS = $(PRIVATEHEADERS)
PRIVCOPYHEADERS = $(patsubst %.h, \
		    $(top_srcdir)/include/%.h, \
		    $(notdir $(PRIVREALHEADERS)))
REALHEADERS = $(filter-out \
		$(PRIVREALHEADERS), \
		$(filter %.h,$(libapt_pkg_la_SOURCES)))
COPYHEADERS = $(patsubst %.h, \
		$(top_srcdir)/include/apt-pkg/%.h, \
		$(notdir $(REALHEADERS)))
REALOBJECTS = $(patsubst %.cc,%.lo,\
	      	$(filter %.cc,$(libapt_pkg_la_SOURCES)))
$(REALOBJECTS): $(COPYHEADERS) $(PRIVCOPYHEADERS)
$(COPYHEADERS): $(REALHEADERS)
	@echo "Updating include/apt-pkg/ header directory."
	@mkdir -p $(top_srcdir)/include/apt-pkg
	@for file in $?; do cp -f $$file $(top_srcdir)/include/apt-pkg; done
$(PRIVCOPYHEADERS): $(PRIVREALHEADERS)
	@echo "Updating include/ header directory."
	@mkdir -p $(top_srcdir)/include
	@for file in $?; do cp -f $$file $(top_srcdir)/include; done
clean-local:
	rm -f $(PRIVCOPYHEADERS)
	rm -rf $(top_srcdir)/include/apt-pkg

