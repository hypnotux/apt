
/*
 ######################################################################

 RPM database and hdlist related handling

 ######################################################################
 */

#include <config.h>

#ifdef HAVE_RPM

#include <fcntl.h>
#include <unistd.h>

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>

#include <apt-pkg/rpmhandler.h>

#include <apti18n.h>

#ifdef HAVE_RPM41
#include <rpm/rpmts.h>
#include <rpm/rpmdb.h>
#define rpmxxInitIterator(a,b,c,d) rpmtsInitIterator(a,(rpmTag)b,c,d)
#else
#define rpmxxInitIterator(a,b,c,d) rpmdbInitIterator(a,b,c,d)
#endif

RPMFileHandler::RPMFileHandler(string File)
{
   FD = Fopen(File.c_str(), "r");
   if (FD == NULL)
   {
      /*
      _error->Error(_("could not open RPM package list file %s: %s"),
		    File.c_str(), rpmErrorString());
      */
      return;
   }
   iSize = fdSize(FD);
}

RPMFileHandler::RPMFileHandler(FileFd *File)
{
   FD = fdDup(File->Fd());
   if (FD == NULL)
   {
      /*
      _error->Error(_("could not create RPM file descriptor: %s"),
		    rpmErrorString());
      */
      return;
   }
   iSize = fdSize(FD);
}

RPMFileHandler::~RPMFileHandler()
{
   if (HeaderP != NULL)
      headerFree(HeaderP);
   if (FD != NULL)
      Fclose(FD);
}

bool RPMFileHandler::Skip()
{
   if (FD == NULL)
      return false;
   iOffset = lseek(Fileno(FD),0,SEEK_CUR);
   if (HeaderP != NULL)
       headerFree(HeaderP);
   HeaderP = headerRead(FD, HEADER_MAGIC_YES);
   return (HeaderP != NULL);
}

bool RPMFileHandler::Jump(unsigned Offset)
{
   if (FD == NULL)
      return false;
   if (lseek(Fileno(FD),Offset,SEEK_SET) != Offset)
      return false;
   return Skip();
}

void RPMFileHandler::Rewind()
{
   if (FD == NULL)
      return;
   iOffset = lseek(Fileno(FD),0,SEEK_SET);
   if (iOffset != 0)
      _error->Error(_("could not rewind RPMFileHandler"));
}

RPMDBHandler::RPMDBHandler(bool WriteLock)
	: WriteLock(WriteLock)
{
   string Dir = _config->Find("RPM::RootDir");
   rpmReadConfigFiles(NULL, NULL);
#ifdef HAVE_RPM4
   RpmIter = NULL;
#endif
#ifdef HAVE_RPM41   
   Handler = rpmtsCreate();
   if (!Dir.empty())
      rpmtsSetRootDir(Handler, Dir.c_str());
   if (rpmtsOpenDB(Handler, WriteLock?O_RDWR:O_RDONLY) != 0)
   {
      _error->Error(_("could not open RPM database"));
      return;
   }
#else
   const char *RootDir = NULL;
   if (!Dir.empty())
      RootDir = Dir.c_str();
   if (rpmdbOpen(RootDir, &Handler, WriteLock?O_RDWR:O_RDONLY, 0644) != 0)
   {
      _error->Error(_("could not open RPM database"));
      return;
   }
#endif
#ifdef HAVE_RPM4
   RpmIter = rpmxxInitIterator(Handler, RPMDBI_PACKAGES, NULL, 0);
   if (RpmIter == NULL) {
      _error->Error(_("could not create RPM database iterator"));
      return;
   }
   iSize = rpmdbGetIteratorCount(RpmIter);
#else
   struct stat st;
   stat(DataPath(false).c_str(), &st);
   iSize = st.st_size;
#endif
}

RPMDBHandler::~RPMDBHandler()
{
#ifdef HAVE_RPM4
   if (RpmIter == NULL)
      return;
   rpmdbFreeIterator(RpmIter);
   RpmIter = NULL;
#else
   if (HeaderP != NULL)
       headerFree(HeaderP);
#endif

#ifdef HAVE_RPM41   
   rpmtsFree(Handler);
#else
   rpmdbClose(Handler);
#endif
}

string RPMDBHandler::DataPath(bool DirectoryOnly)
{
   string File = "packages.rpm";
#ifdef HAVE_RPM4
   if (rpmExpandNumeric("%{_dbapi}") >= 3)
      File = "Packages";       
#endif
   if (DirectoryOnly == true)
       return _config->Find("RPM::RootDir")+"/var/lib/rpm";
   else
       return _config->Find("RPM::RootDir")+"/var/lib/rpm/"+File;
}

bool RPMDBHandler::Skip()
{
#ifdef HAVE_RPM4
   if (RpmIter == NULL)
       return false;
   HeaderP = rpmdbNextIterator(RpmIter);
   iOffset = rpmdbGetIteratorOffset(RpmIter);
   if (HeaderP == NULL)
      return false;
#else
   if (iOffset == 0)
      iOffset = rpmdbFirstRecNum(Handler);
   else
      iOffset = rpmdbNextRecNum(Handler, iOffset);
   if (HeaderP != NULL)
   {
      headerFree(HeaderP);
      HeaderP = NULL;
   }
   if (iOffset == 0)
       return false;
   HeaderP = rpmdbGetRecord(Handler, iOffset);
#endif
   return true;
}

bool RPMDBHandler::Jump(unsigned int Offset)
{
   iOffset = Offset;
#ifdef HAVE_RPM4
   if (RpmIter == NULL)
      return false;
   rpmdbFreeIterator(RpmIter);
   if (iOffset == 0)
      RpmIter = rpmxxInitIterator(Handler, RPMDBI_PACKAGES, NULL, 0);
   else
      RpmIter = rpmxxInitIterator(Handler, RPMDBI_PACKAGES,
				  &iOffset, sizeof(iOffset));
   HeaderP = rpmdbNextIterator(RpmIter);
#else
   HeaderP = rpmdbGetRecord(Handler, iOffset);
#endif
   return true;
}

void RPMDBHandler::Rewind()
{
#ifdef HAVE_RPM4
   if (RpmIter == NULL)
      return;
   rpmdbFreeIterator(RpmIter);   
   RpmIter = rpmxxInitIterator(Handler, RPMDBI_PACKAGES, NULL, 0);
#else
   if (HeaderP != NULL)
   {
      headerFree(HeaderP);
      HeaderP = NULL;
   }
#endif
   iOffset = 0;
}
#endif

// vim:sts=3:sw=3
