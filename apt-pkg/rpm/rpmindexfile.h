// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id $
/* ######################################################################

   RPM Index Files
   
   There are three sorts currently
   
   pkglist files
   The system RPM database
   srclist files
   
   ##################################################################### 
 */
									/*}}}*/
#ifndef PKGLIB_RPMINDEXFILE_H
#define PKGLIB_RPMINDEXFILE_H

#ifdef __GNUG__
#pragma interface "apt-pkg/rpmindexfile.h"
#endif

#include <apt-pkg/indexfile.h>

class RPMHandler;
class RPMDBHandler;
class pkgRepository;

class rpmIndexFile : public pkgIndexFile
{
   
   public:

   virtual RPMHandler *CreateHandler() const = 0;

};

class rpmDatabaseIndex : public rpmIndexFile
{
   public:

   virtual const Type *GetType() const;

   // Creates a RPMHandler suitable for usage with this object
   virtual RPMHandler *CreateHandler() const;
   
   // Interface for acquire
   virtual string Describe(bool Short) const {return "RPM Database";};
   
   // Interface for the Cache Generator
   virtual bool Exists() const {return true;};
   virtual bool HasPackages() const {return true;};
   virtual unsigned long Size() const;
   virtual bool Merge(pkgCacheGenerator &Gen,OpProgress &Prog) const;
   virtual bool MergeFileProvides(pkgCacheGenerator &/*Gen*/,
		   		  OpProgress &/*Prog*/) const;
   virtual pkgCache::PkgFileIterator FindInCache(pkgCache &Cache) const;

   rpmDatabaseIndex();
};

class rpmListIndex : public pkgIndexFile
{

   protected:

   string URI;
   string Dist;
   string Section;
   pkgRepository *Repository;
   
   string ReleaseFile(string Type) const;
   string ReleaseURI(string Type) const;   
   string ReleaseInfo(string Type) const;   

   public:

   bool GetReleases(pkgAcquire *Owner) const;

   rpmListIndex(string URI,string Dist,string Section,
		pkgRepository *Repository) :
               	URI(URI), Dist(Dist), Section(Section),
   		Repository(Repository)
	{};
};

class rpmPkgListIndex : public rpmListIndex
{
   string Info(string Type) const;
   string IndexFile(string Type) const;
   string IndexURI(string Type) const;   
   
   public:

   virtual const Type *GetType() const;
   
   // Creates a RPMHandler suitable for usage with this object
   virtual RPMHandler *CreateHandler() const;

   // Stuff for accessing files on remote items
   virtual string ArchiveInfo(pkgCache::VerIterator Ver) const;
   virtual string ArchiveURI(string File) const;
   
   // Interface for acquire
   virtual string Describe(bool Short) const;   
   virtual bool GetIndexes(pkgAcquire *Owner) const;
   
   // Interface for the Cache Generator
   virtual bool Exists() const;
   virtual bool HasPackages() const {return true;};
   virtual unsigned long Size() const;
   virtual bool Merge(pkgCacheGenerator &Gen,OpProgress &Prog) const;
   virtual bool MergeFileProvides(pkgCacheGenerator &/*Gen*/,
		   		  OpProgress &/*Prog*/) const;
   virtual pkgCache::PkgFileIterator FindInCache(pkgCache &Cache) const;

   rpmPkgListIndex(string URI,string Dist,string Section,
		   pkgRepository *Repository) :
	   rpmListIndex(URI,Dist,Section,Repository)
      {};
};


class rpmSrcListIndex : public rpmListIndex
{
   string Info(string Type) const;
   string IndexFile(string Type) const;
   string IndexURI(string Type) const;   
   
   public:

   virtual const Type *GetType() const;

   // Creates a RPMHandler suitable for usage with this object
   virtual RPMHandler *CreateHandler() const;

   // Stuff for accessing files on remote items
   virtual string SourceInfo(pkgSrcRecords::Parser const &Record,
			     pkgSrcRecords::File const &File) const;
   virtual string ArchiveURI(string File) const;
   
   // Interface for acquire
   virtual string Describe(bool Short) const;   
   virtual bool GetIndexes(pkgAcquire *Owner) const;

   // Interface for the record parsers
   virtual pkgSrcRecords::Parser *CreateSrcParser() const;
   
   // Interface for the Cache Generator
   virtual bool Exists() const;
   virtual bool HasPackages() const {return false;};
   virtual unsigned long Size() const;

   rpmSrcListIndex(string URI,string Dist,string Section,
		   pkgRepository *Repository) :
	   rpmListIndex(URI,Dist,Section,Repository)
      {};
};

#endif
