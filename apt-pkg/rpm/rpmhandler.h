
/* ######################################################################
  
   RPM database and hdlist related handling

   ######################################################################
 */


#ifndef PKGLIB_RPMHANDLER_H
#define PKGLIB_RPMHANDLER_H

#include <apt-pkg/fileutl.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>

#include <config.h>

class RPMHandler
{
   protected:

   unsigned int iOffset;
   unsigned int iSize;
   Header HeaderP;

   public:

   virtual bool Skip() = 0;
   virtual bool Jump(unsigned int Offset) = 0;
   virtual void Rewind() = 0;
   inline unsigned Offset() {return iOffset;};
   virtual bool OrderedOffset() {return true;};
   inline unsigned Size() {return iSize;};
   inline Header GetHeader() {return HeaderP;};
   virtual bool IsDatabase() = 0;

   RPMHandler() : iOffset(0), iSize(0), HeaderP(0) {};
   virtual ~RPMHandler() {};
};


class RPMFileHandler : public RPMHandler
{   
   FD_t FD;

   public:

   virtual bool Skip();
   virtual bool Jump(unsigned int Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return false;};

   RPMFileHandler(FileFd *File);
   RPMFileHandler(string File);
   virtual ~RPMFileHandler();
};


class RPMDBHandler : public RPMHandler
{
#ifdef HAVE_RPM41
   rpmts Handler;
#else
   rpmdb Handler;
#endif
#ifdef HAVE_RPM4
   rpmdbMatchIterator RpmIter;
#endif
   bool WriteLock;

   time_t DbFileMtime;

   public:

   static string DataPath(bool DirectoryOnly=true);
   virtual bool Skip();
   virtual bool Jump(unsigned Offset);
   virtual void Rewind();
   virtual inline bool IsDatabase() {return true;};
   virtual bool HasWriteLock() {return WriteLock;};
   virtual time_t Mtime() {return DbFileMtime;}
   virtual bool OrderedOffset() {return false;};

   RPMDBHandler(bool WriteLock=false);
   virtual ~RPMDBHandler();
};


// Our Extra RPM tags
#define CRPMTAG_FILENAME          1000000
#define CRPMTAG_FILESIZE          1000001
#define CRPMTAG_MD5               1000005
#define CRPMTAG_SHA1              1000006

#define CRPMTAG_DIRECTORY         1000010
#define CRPMTAG_BINARY            1000011

#define CRPMTAG_UPDATE_SUMMARY    1000020
#define CRPMTAG_UPDATE_IMPORTANCE 1000021
#define CRPMTAG_UPDATE_DATE       1000022
#define CRPMTAG_UPDATE_URL        1000023

#endif
