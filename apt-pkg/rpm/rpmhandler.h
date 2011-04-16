
/* ######################################################################
  
   RPM database and hdlist related handling

   ######################################################################
 */


#ifndef PKGLIB_RPMHANDLER_H
#define PKGLIB_RPMHANDLER_H

#include <apt-pkg/aptconf.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/pkgrecords.h>

#ifdef APT_WITH_REPOMD
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include "sqlite.h"
#endif

#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>
#include "rapttypes.h"

#include <sys/types.h>
#include <dirent.h>

#include <vector>

// Our Extra RPM tags. These should not be accessed directly. Use
// the methods in RPMHandler instead.
#define CRPMTAG_FILENAME          (rpmTag)1000000
#define CRPMTAG_FILESIZE          (rpmTag)1000001
#define CRPMTAG_MD5               (rpmTag)1000005
#define CRPMTAG_SHA1              (rpmTag)1000006

#define CRPMTAG_DIRECTORY         (rpmTag)1000010
#define CRPMTAG_BINARY            (rpmTag)1000011

using std::string;
using std::vector;

struct Dependency
{
   string Name;
   string Version;
   unsigned int Op;
   unsigned int Type;
};

class RPMHandler
{
   protected:

   off_t iOffset;
   off_t iSize;
   string ID;

   unsigned int DepOp(raptDepFlags rpmflags) const;
   bool InternalDep(const char *name, const char *ver, raptDepFlags flag) const;
   bool PutDep(const char *name, const char *ver, raptDepFlags flags,
               unsigned int type, vector<Dependency*> &Deps) const;

   public:

   // Return a unique ID for that handler. Actually, implemented used
   // the file/dir name.
   virtual string GetID() { return ID; }

   virtual bool Skip() = 0;
   virtual bool Jump(off_t Offset) = 0;
   virtual void Rewind() = 0;
   inline unsigned Offset() const {return iOffset;}
   virtual bool OrderedOffset() const {return true;}
   inline unsigned Size() {return iSize;}
   virtual bool IsDatabase() const {return false;};

   virtual string FileName() const = 0;
   virtual string Directory() const = 0;
   virtual off_t FileSize() const = 0;
   virtual string Hash() const = 0;
   virtual string HashType() const = 0;
   virtual bool ProvideFileName() {return false;}

   virtual string Name() const = 0;
   virtual string Arch() const = 0;
   virtual string Epoch() const = 0;
   virtual string Version() const = 0;
   virtual string Release() const = 0;
   virtual string EVR() const;
   virtual string Group()  const = 0;
   virtual string Packager() const = 0;
   virtual string Vendor() const = 0;
   virtual string Summary() const = 0;
   virtual string Description() const = 0;
   virtual off_t InstalledSize() const = 0;
   virtual string SourceRpm() const = 0;
   virtual bool IsSourceRpm() const {return SourceRpm().empty();}

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps) const = 0;
   virtual bool FileList(vector<string> &FileList) const = 0;
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs) const = 0;

   virtual bool HasFile(const char *File) const;

   RPMHandler() : iOffset(0), iSize(0) {}
   virtual ~RPMHandler() {}
};

class RPMHdrHandler : public RPMHandler
{
   protected:

   Header HeaderP;

   string GetSTag(raptTag Tag) const;
   off_t GetITag(raptTag Tag) const;

   public:

   virtual string FileName() const {return "";}
   virtual string Directory() const {return "";}
   virtual off_t FileSize() const {return 1;}
   virtual string Hash() const {return "";};
   virtual string HashType() const {return "";};
   virtual bool ProvideFileName() const {return false;}

   virtual string Name() const {return GetSTag(RPMTAG_NAME);}
   virtual string Arch() const {return GetSTag(RPMTAG_ARCH);}
   virtual string Epoch() const;
   virtual string Version() const {return GetSTag(RPMTAG_VERSION);}
   virtual string Release() const {return GetSTag(RPMTAG_RELEASE);}
   virtual string Group() const {return GetSTag(RPMTAG_GROUP);}
   virtual string Packager() const {return GetSTag(RPMTAG_PACKAGER);}
   virtual string Vendor() const {return GetSTag(RPMTAG_VENDOR);}
   virtual string Summary() const {return GetSTag(RPMTAG_SUMMARY);}
   virtual string Description() const {return GetSTag(RPMTAG_DESCRIPTION);}
   virtual off_t InstalledSize() const {return GetITag(RPMTAG_SIZE);}
   virtual string SourceRpm() const {return GetSTag(RPMTAG_SOURCERPM);}
   virtual bool IsSourceRpm() const {return SourceRpm().empty();}

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps) const;
   virtual bool FileList(vector<string> &FileList) const ;
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs) const;

   RPMHdrHandler() : RPMHandler(), HeaderP(0) {}
   virtual ~RPMHdrHandler() {}
};


class RPMFileHandler : public RPMHdrHandler
{   
   protected:

   FD_t FD;

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual bool OrderedOffset() const {return true;}

   virtual string FileName() const;
   virtual string Directory() const;
   virtual off_t FileSize() const;
   virtual string Hash() const;
   virtual string HashType() const;

   // the rpm-repotype stripped down hdrlists dont carry changelog data
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs)  const{ return false; }

   RPMFileHandler(FileFd *File);
   RPMFileHandler(string File);
   virtual ~RPMFileHandler();
};

class RPMSingleFileHandler : public RPMFileHandler
{   
   private:

   string sFilePath;

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();

   virtual string FileName() const {return sFilePath;}
   virtual string Directory() const {return "";}
   virtual off_t FileSize() const;
   virtual string Hash() const;
   virtual bool ProvideFileName() const {return true;}
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs) const;

   RPMSingleFileHandler(string File) : RPMFileHandler(File), sFilePath(File) {}
   virtual ~RPMSingleFileHandler() {}
};

class RPMDBHandler : public RPMHdrHandler
{
   private:

   rpmts Handler;
   rpmdbMatchIterator RpmIter;
   bool WriteLock;

   time_t DbFileMtime;

   public:

   static string DataPath(bool DirectoryOnly=true);
   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();
   virtual bool IsDatabase() const {return true;}
   virtual bool HasWriteLock() {return WriteLock;}
   virtual time_t Mtime() {return DbFileMtime;}
   virtual bool OrderedOffset() const {return false;}

   // used by rpmSystem::DistroVer()
   bool JumpByName(string PkgName, bool Provides=false);

   RPMDBHandler(bool WriteLock=false);
   virtual ~RPMDBHandler();
};

class RPMDirHandler : public RPMHdrHandler
{   
   private:

   DIR *Dir;
   string sDirName;
   string sFileName;
   string sFilePath;

   rpmts TS;

   const char *nextFileName();

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();

   virtual string FileName()  const{return (Dir == NULL)?"":sFileName;}
   virtual off_t FileSize() const;
   virtual string Hash() const;
   virtual string HashType() const;

   RPMDirHandler(string DirName);
   virtual ~RPMDirHandler();
};

#ifdef APT_WITH_REPOMD
class repomdXML;
class RPMRepomdHandler : public RPMHandler
{
   private:
   xmlDocPtr Primary;
   xmlNode *Root;
   xmlNode *NodeP;

   vector<xmlNode *> Pkgs;
   vector<xmlNode *>::iterator PkgIter;

   string PrimaryPath;
   string FilelistPath;
   string OtherPath;

   bool HavePrimary;
   bool LoadPrimary();

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();

   virtual string FileName() const;
   virtual string Directory() const;
   virtual off_t FileSize() const;
   virtual off_t InstalledSize() const;
   virtual string Hash() const;
   virtual string HashType() const;

   virtual string Name() const;
   virtual string Arch() const;
   virtual string Epoch() const;
   virtual string Version() const;
   virtual string Release() const;

   virtual string Group() const;
   virtual string Packager() const;
   virtual string Vendor() const;
   virtual string Summary() const;
   virtual string Description() const;
   virtual string SourceRpm() const;

   virtual bool HasFile(const char *File) const;
   virtual bool ShortFileList(vector<string> &FileList) const;

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps) const;
   virtual bool FileList(vector<string> &FileList) const;
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs) const;

   RPMRepomdHandler(repomdXML const *repomd);
   virtual ~RPMRepomdHandler();
};

class RPMRepomdReaderHandler : public RPMHandler
{
   protected:
   xmlTextReaderPtr XmlFile;
   string XmlPath;
   xmlNode *NodeP;

   string FindTag(const char *Tag) const;
   string FindVerTag(const char *Tag) const;

   public:
   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();

   virtual string FileName() const {return XmlPath;}
   virtual string Directory() const {return "";}
   virtual off_t FileSize() const {return 0;}
   virtual off_t InstalledSize() const {return 0;}
   virtual string Hash() const {return "";}
   virtual string HashType() const {return "";};

   virtual string Name() const {return FindTag("name");}
   virtual string Arch() const {return FindTag("arch");}
   virtual string Epoch() const {return FindVerTag("epoch");}
   virtual string Version() const {return FindVerTag("ver");}
   virtual string Release() const {return FindVerTag("rel");}

   virtual string Group() const {return "";}
   virtual string Packager() const {return "";}
   virtual string Vendor() const {return "";}
   virtual string Summary() const {return "";}
   virtual string Description() const {return "";}
   virtual string SourceRpm() const {return "";}
   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps) const
       {return true;};

   RPMRepomdReaderHandler(string File);
   virtual ~RPMRepomdReaderHandler();
};

class RPMRepomdFLHandler : public RPMRepomdReaderHandler
{
   public:
   virtual bool FileList(vector<string> &FileList) const;
   RPMRepomdFLHandler(string File) : RPMRepomdReaderHandler(File) {}
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs)  const{return true;}
   virtual ~RPMRepomdFLHandler() {}
};

class RPMRepomdOtherHandler : public RPMRepomdReaderHandler
{
   public:
   virtual bool FileList(vector<string> &FileList)  const{return true;}
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs) const;

   RPMRepomdOtherHandler(string File) : RPMRepomdReaderHandler(File) {}
   virtual ~RPMRepomdOtherHandler() {}
};

#ifdef WITH_SQLITE3
class RPMSqliteHandler : public RPMHandler
{
   private:

   SqliteDB *Primary;
   SqliteDB *Filelists;
   SqliteDB *Other;
   
   SqliteQuery *Packages;

   SqliteQuery *Provides;
   SqliteQuery *Requires;
   SqliteQuery *Conflicts;
   SqliteQuery *Obsoletes;

   SqliteQuery *Files;
   SqliteQuery *Changes;
  
   string DBPath;
   string FilesDBPath;
   string OtherDBPath;

   int DBVersion;

   public:

   virtual bool Skip();
   virtual bool Jump(off_t Offset);
   virtual void Rewind();

   virtual string FileName() const;
   virtual string Directory() const;
   virtual off_t FileSize() const;
   virtual off_t InstalledSize() const;
   virtual string Hash() const;
   virtual string HashType() const;

   virtual string Name() const;
   virtual string Arch() const;
   virtual string Epoch() const;
   virtual string Version() const;
   virtual string Release() const;

   virtual string Group() const;
   virtual string Packager() const;
   virtual string Vendor() const;
   virtual string Summary() const;
   virtual string Description() const;
   virtual string SourceRpm() const;

   virtual bool PRCO(unsigned int Type, vector<Dependency*> &Deps) const;
   virtual bool FileList(vector<string> &FileList) const;
   virtual bool ChangeLog(vector<ChangeLogEntry* > &ChangeLogs) const;

   RPMSqliteHandler(repomdXML const *repomd);
   virtual ~RPMSqliteHandler();
};
#endif /* WITH_SQLITE3 */

#endif /* APT_WITH_REPOMD */

#endif
// vim:sts=3:sw=3
