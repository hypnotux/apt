%module apt
%include std_string.i
%include std_vector.i

%{
#include <apt-pkg/init.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/mmap.h>
#include <apt-pkg/cacheiterators.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/contrib/configuration.h>
#include <apt-pkg/contrib/progress.h>
#include <apt-pkg/version.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/packagemanager.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/contrib/error.h>

#include <string>
#include <time.h>
%}

%inline %{
typedef pkgCache::VerIterator VerIterator;
typedef pkgCache::PkgIterator PkgIterator;
typedef pkgCache::DepIterator DepIterator;
typedef pkgCache::PrvIterator PrvIterator;
typedef pkgCache::PkgFileIterator PkgFileIterator;
typedef pkgCache::VerFileIterator VerFileIterator;
typedef pkgCache::Header Header;
typedef pkgCache::Package Package;
typedef pkgCache::PackageFile PackageFile;
typedef pkgCache::VerFile VerFile;
typedef pkgCache::Version Version;
typedef pkgCache::Dependency Dependency;
typedef pkgCache::Provides Provides;
typedef pkgCache::StringItem StringItem;
typedef pkgCache::Dep Dep;
typedef pkgCache::State State;
typedef pkgCache::Flag Flag;
typedef pkgDepCache::Policy Policy;
typedef pkgDepCache::StateCache StateCache;
typedef Configuration::Item Item;
typedef pkgRecords::Parser Parser;
%}

/* Fix some operators. */
%rename(next) operator++;
%rename(assign) operator=;
%rename(pkgCache) operator pkgCache *;
%rename(pkgDepCache) operator pkgDepCache *;
%rename(Package) operator Package *;
%rename(Version) operator Version *;
%rename(Dependency) operator Dependency *;
%rename(Provides) operator Provides *;
%rename(PackageFile) operator PackageFile *;
%rename(VerFile) operator VerFile *;
%rename(__getitem__) operator[];
%ignore operator pkgCache &;
%ignore operator pkgDepCache &;

/* Set some data as immutable. */
%immutable pkgVersion;
%immutable pkgLibVersion;
%immutable pkgOS;
%immutable pkgCPU;
%immutable pkgSystem::Label;
%immutable pkgVersioningSystem::Label;
%immutable pkgDepCache::StateCache::CandVersion;
%immutable pkgDepCache::StateCache::CurVersion;

/* One-shot initialization function. */
%inline %{
inline bool pkgInit() 
{
   return pkgInitConfig(*_config) && pkgInitSystem(*_config,_system);
}
%}

/* No suport for nested classes yet. */
%rename(pkgCacheHeader) pkgCache::Header;
%rename(pkgCachePackage) pkgCache::Package;
%rename(pkgCachePackageFile) pkgCache::PackageFile;
%rename(pkgCacheVerFile) pkgCache::VerFile;
%rename(pkgCacheVersion) pkgCache::Version;
%rename(pkgCacheDependency) pkgCache::Dependency;
%rename(pkgCacheProvides) pkgCache::Provides;
%rename(pkgCacheStringItem) pkgCache::StringItem;
%rename(pkgCacheDep) pkgCache::Dep;
%rename(pkgCacheState) pkgCache::State;
%rename(pkgCacheFlag) pkgCache::Flag;
%rename(pkgCacheVerIterator) pkgCache::VerIterator;
%rename(pkgCachePkgIterator) pkgCache::PkgIterator;
%rename(pkgCacheDepIterator) pkgCache::DepIterator;
%rename(pkgCachePrvIterator) pkgCache::PrvIterator;
%rename(pkgCachePkgFileIterator) pkgCache::PkgFileIterator;
%rename(pkgCacheVerFileIterator) pkgCache::VerFileIterator;
%rename(pkgDepCacheStateCache) pkgDepCache::StateCache;
%rename(pkgRecordsParser) pkgRecords::Parser;
%rename(pkgAcquireItem) pkgAcquire::Item;
%rename(ConfigurationItem) Configuration::Item;

/* That's the best way I found to access ItemsBegin/ItemsEnd. */
%ignore pkgAcquire::ItemsBegin;
%ignore pkgAcquire::ItemsEnd;
%extend pkgAcquire {
PyObject *
ItemsList()
{
	static swig_type_info *ItemDescr = NULL;
	PyObject *list, *o;
	pkgAcquire::ItemIterator I;
	if (!ItemDescr) {
		ItemDescr = SWIG_TypeQuery("pkgAcquire::Item *");
		assert(ItemDescr);
	}
	list = PyList_New(0);
	if (list == NULL)
		return NULL;
	for (I = self->ItemsBegin(); I != self->ItemsEnd(); I++) {
		o = SWIG_NewPointerObj((void *)(*I), ItemDescr, 0);
		if (!o || PyList_Append(list, o) == -1) {
			Py_XDECREF(o);
			Py_DECREF(list);
			return NULL;
	    	}
		Py_DECREF(o);
	}
	return list;
}
}

/* Wrap string members. */
%immutable pkgAcquire::Item::DestFile;
%immutable pkgAcquire::Item::ErrorText;
%extend pkgAcquire::Item {
	const char *DestFile;
	const char *ErrorText;
}
%ignore pkgAcquire::Item::DestFile;
%ignore pkgAcquire::Item::ErrorText;
%{
#define pkgAcquire_Item_DestFile_get(x) ((x)->DestFile.c_str())
#define pkgAcquire_Item_ErrorText_get(x) ((x)->ErrorText.c_str())
%}

/* Also from Configuration::Item. */
%extend Configuration::Item {
	const char *Tag;
	const char *Value;
}
%ignore Configuration::Item::Tag;
%ignore Configuration::Item::Value;
%{
#define Configuration_Item_Tag_get(x) ((x)->Tag.c_str())
#define Configuration_Item_Value_get(x) ((x)->Value.c_str())
#define Configuration_Item_Tag_set(x,y) ((x)->Tag = (y))
#define Configuration_Item_Value_set(x,y) ((x)->Value = (y))
%}

/* Typemap to present map_ptrloc in a better way */
%apply int { map_ptrloc };

/* That should be enough for our usage, but _error is indeed an alias
 * for a function which returns an statically alocated GlobalError object. */
%immutable _error;
GlobalError *_error;

/* Undefined reference!? */
%ignore pkgCache::PkgIterator::TargetVer;

/* There's a struct and a function with the same name. */
%ignore SubstVar;

/* Preprocess string macros (note that we're %import'ing). */
%import <apt-pkg/contrib/strutl.h>

%include <apt-pkg/init.h>
%include <apt-pkg/pkgcache.h>
%include <apt-pkg/depcache.h>
%include <apt-pkg/cacheiterators.h>
%include <apt-pkg/cachefile.h>
%include <apt-pkg/algorithms.h>
%include <apt-pkg/pkgsystem.h>
%include <apt-pkg/contrib/configuration.h>
%include <apt-pkg/contrib/progress.h>
%include <apt-pkg/version.h>
%include <apt-pkg/pkgrecords.h>
%include <apt-pkg/acquire-item.h>
%include <apt-pkg/acquire.h>
%include <apt-pkg/packagemanager.h>
%include <apt-pkg/sourcelist.h>
%include <apt-pkg/contrib/error.h>

/* Create a dumb status class which can be instantiated. pkgAcquireStatus
 * has fully abstract methods. */
%inline %{
class pkgAcquireStatusDumb : public pkgAcquireStatus
{
   virtual bool MediaChange(string Media,string Drive) {};
};
%}

// vim:ft=swig
