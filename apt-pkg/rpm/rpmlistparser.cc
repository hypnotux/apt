// Description
// $Id: rpmlistparser.cc,v 1.7 2003/01/29 18:55:03 niemeyer Exp $
// 
/* ######################################################################
 * 
 * Package Cache Generator - Generator for the cache structure.
 * This builds the cache structure from the abstract package list parser. 
 * 
 ##################################################################### 
 */


// Include Files
#include <config.h>

#ifdef HAVE_RPM

#include <apt-pkg/rpmlistparser.h>
#include <apt-pkg/rpmhandler.h>
#include <apt-pkg/rpmpackagedata.h>
#include <apt-pkg/rpmsystem.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/crc-16.h>
#include <apt-pkg/tagfile.h>

#include <apti18n.h>

#include <rpm/rpmlib.h>

#ifdef HAVE_RPM41
#include <rpm/rpmds.h>
#endif

// ListParser::rpmListParser - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* */
rpmListParser::rpmListParser(RPMHandler *Handler)
	: Handler(Handler)
{
   Handler->Rewind();
   header = NULL;
   if (Handler->IsDatabase() == true)
       SeenPackages = new map<string,unsigned long>();
   else
       SeenPackages = NULL;
   RpmData = RPMPackageData::Singleton();
}
                                                                        /*}}}*/

rpmListParser::~rpmListParser()
{
   delete SeenPackages;
}

// ListParser::UniqFindTagWrite - Find the tag and write a unq string	/*{{{*/
// ---------------------------------------------------------------------
/* */
unsigned long rpmListParser::UniqFindTagWrite(int Tag)
{
   char *Start;
   char *Stop;
   int type;
   int count;
   void *data;
   
   if (headerGetEntry(header, Tag, &type, &data, &count) != 1)
      return 0;
   
   if (type == RPM_STRING_TYPE) 
   {
      Start = (char*)data;
      Stop = Start + strlen(Start);
   } else {
      cout << "oh shit, not handled:"<<type<<" Package:"<<Package()<<endl;
      abort();
   }
   
   return WriteUniqString(Start,Stop - Start);
}
                                                                        /*}}}*/
// ListParser::Package - Return the package name			/*{{{*/
// ---------------------------------------------------------------------
/* This is to return the name of the package this section describes */
string rpmListParser::Package()
{
   if (CurrentName.empty() == false)
      return CurrentName;

   char *str;
   int type, count;
   
   Duplicated = false;
   
   if (headerGetEntry(header, RPMTAG_NAME, &type, (void**)&str, &count) != 1) 
   {
      _error->Error("Corrupt pkglist: no RPMTAG_NAME in header entry");
      return "";
   } 

   bool IsDup = false;
   string Name = str;
   
   // If this package can have multiple versions installed at
   // the same time, then we make it so that the name of the
   // package is NAME+"#"+VERSION and also add a provides
   // with the original name and version, to satisfy the 
   // dependencies.
   if (RpmData->IsDupPackage(Name) == true)
      IsDup = true;
   else if (SeenPackages != NULL) {
      if (SeenPackages->find(Name) != SeenPackages->end() &&
	  (*SeenPackages)[Name] != Offset())
      {
	 if (_config->FindB("RPM::Allow-Duplicated-Warning", true) == true)
	    _error->Warning(
   _("There are multiple versions of \"%s\" in your system.\n"
     "\n"
     "This package won't be cleanly updated, unless you leave\n"
     "only one version. To leave multiple versions installed,\n"
     "you may remove that warning by setting the following\n"
     "option in your configuration file:\n"
     "\n"
     "RPM::Allow-Duplicated { \"^%s$\"; };\n"
     "\n"
     "To disable these warnings completely set:\n"
     "\n"
     "RPM::Allow-Duplicated-Warning \"false\";\n")
			      , Name.c_str(), Name.c_str());
	 RpmData->SetDupPackage(Name);
	 VirtualizePackage(Name);
	 IsDup = true;
      }
      (*SeenPackages)[Name] = Offset();
   }
   if (IsDup == true)
   {
      Name += "#"+Version();
      Duplicated = true;
   } 
   CurrentName = Name;
   return Name;
}
                                                                        /*}}}*/
// ListParser::Arch - Return the architecture string			/*{{{*/
// ---------------------------------------------------------------------
string rpmListParser::Architecture()
{
    int type, count;
    char *arch;
    int res;
    res = headerGetEntry(header, RPMTAG_ARCH, &type, (void **)&arch, &count);
    return string(res?arch:"");
}
                                                                        /*}}}*/
// ListParser::Version - Return the version string			/*{{{*/
// ---------------------------------------------------------------------
/* This is to return the string describing the version in RPM form,
 version-release. If this returns the blank string then the
 entry is assumed to only describe package properties */
string rpmListParser::Version()
{
   char *ver, *rel;
   int_32 *ser;
   bool has_epoch = false;
   int type, count;
   string str;
   str.reserve(10);

   if (headerGetEntry(header, RPMTAG_EPOCH, &type, (void **)&ser, &count) == 1
       && count > 0) 
      has_epoch = true;
   
   headerGetEntry(header, RPMTAG_VERSION, &type, (void **)&ver, &count);
   headerGetEntry(header, RPMTAG_RELEASE, &type, (void **)&rel, &count);

   if (has_epoch == true) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%i", ser[0]);
      str += buf;
      str += ":";
      str += ver;
      str += "-";
      str += rel;
   }
   else {
      str += ver;
      str += "-";
      str += rel;
   }
   return str;
}
                                                                        /*}}}*/
// ListParser::NewVersion - Fill in the version structure		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmListParser::NewVersion(pkgCache::VerIterator Ver)
{
   int count, type;
   int_32 *num;
   
   // Parse the section
   Ver->Section = UniqFindTagWrite(RPMTAG_GROUP);
   Ver->Arch = UniqFindTagWrite(RPMTAG_ARCH);
   
   // Archive Size
   Ver->Size = Handler->FileSize();
   
   // Unpacked Size (in kbytes)
   headerGetEntry(header, RPMTAG_SIZE, &type, (void**)&num, &count);
   Ver->InstalledSize = (unsigned)num[0];
     
   if (ParseDepends(Ver,pkgCache::Dep::Depends) == false)
       return false;
   if (ParseDepends(Ver,pkgCache::Dep::PreDepends) == false)
       return false;
   if (ParseDepends(Ver,pkgCache::Dep::Conflicts) == false)
       return false;
   if (ParseDepends(Ver,pkgCache::Dep::Obsoletes) == false)
       return false;
#ifdef OLD_FILEDEPS
   if (ProcessFileProvides(Ver) == false)
       return false;
#endif

   if (ParseProvides(Ver) == false)
       return false;

   return true;
}
									/*}}}*/
// ListParser::UsePackage - Update a package structure			/*{{{*/
// ---------------------------------------------------------------------
/* This is called to update the package with any new information
   that might be found in the section */
bool rpmListParser::UsePackage(pkgCache::PkgIterator Pkg,
			       pkgCache::VerIterator Ver)
{
   if (Pkg->Section == 0)
      Pkg->Section = UniqFindTagWrite(RPMTAG_GROUP);
   if (_error->PendingError()) 
       return false;
   Ver->Priority = RpmData->VerPriority(Pkg.Name());
   Pkg->Flags |= RpmData->PkgFlags(Pkg.Name());
   if (ParseStatus(Pkg,Ver) == false)
       return false;
   return true;
}
                                                                        /*}}}*/
// ListParser::VersionHash - Compute a unique hash for this version	/*{{{*/
// ---------------------------------------------------------------------
/* */

static int compare(const void *a, const void *b)
{   
   return strcmp(*(char**)a, *(char**)b);
}


unsigned short rpmListParser::VersionHash()
{
   int Sections[] = {
	  RPMTAG_VERSION,
	  RPMTAG_RELEASE,
	  RPMTAG_ARCH,
	  RPMTAG_REQUIRENAME,
	  RPMTAG_OBSOLETENAME,
	  RPMTAG_CONFLICTNAME,
	  0
   };
   unsigned long Result = INIT_FCS;
   char S[300];
   char *I;
   
   for (const int *sec = Sections; *sec != 0; sec++)
   {
      char *Start;
      char *End;
      int Len;
      int type, count;
      int res;
      char **strings;
      
      res = headerGetEntry(header, *sec, &type, (void **)&strings, &count);
      if (res != 1 || count == 0)
	 continue;
      
      switch (type) 
      {
      case RPM_STRING_ARRAY_TYPE:
	 qsort(strings, count, sizeof(char*), compare);
	 
	 while (count-- > 0) 
	 {
	    Start = strings[count];
	    Len = strlen(Start);
	    End = Start+Len;
	    
	    if (Len >= (signed)sizeof(S))
	       continue;

	    /* Suse patch.rpm hack. */
	    if (*sec == RPMTAG_REQUIRENAME && Len == 17 && *Start == 'r' &&
	        strcmp(Start, "rpmlib(PatchRPMs)") == 0)
	       continue;
	    
	    /* Strip out any spaces from the text */
	    for (I = S; Start != End; Start++) 
	       if (isspace(*Start) == 0)
		  *I++ = *Start;
	    
	    Result = AddCRC16(Result,S,I - S);
	 }
	 break;
	 
      case RPM_STRING_TYPE:	 
	 Start = (char*)strings;
	 Len = strlen(Start);
	 End = Start+Len;
	 
	 if (Len >= (signed)sizeof(S))
	    continue;
	 
	 /* Strip out any spaces from the text */
	 for (I = S; Start != End; Start++) 
	    if (isspace(*Start) == 0)
	       *I++ = *Start;

	 Result = AddCRC16(Result,S,I - S);
	 
	 break;
      }
   }
   
   return Result;
}
                                                                        /*}}}*/
// ListParser::ParseStatus - Parse the status field			/*{{{*/
// ---------------------------------------------------------------------
bool rpmListParser::ParseStatus(pkgCache::PkgIterator Pkg,
				pkgCache::VerIterator Ver)
{   
   if (!Handler->IsDatabase())  // this means we're parsing an hdlist, so it's not installed
      return true;
   
   // if we're reading from the rpmdb, then it's installed
   // 
   Pkg->SelectedState = pkgCache::State::Install;
   Pkg->InstState = pkgCache::State::Ok;
   Pkg->CurrentState = pkgCache::State::Installed;
   
   Pkg->CurrentVer = Ver.Index();
   
   return true;
}


bool rpmListParser::ParseDepends(pkgCache::VerIterator Ver,
				 char **namel, char **verl, int_32 *flagl,
				 int count, unsigned int Type)
{
   int i;
   unsigned int Op = 0;
   
   for (i = 0; i < count; i++) 
   {
      
      if (Type == pkgCache::Dep::Depends) {
	 if (flagl[i] & RPMSENSE_PREREQ)
	     continue;
      } else if (Type == pkgCache::Dep::PreDepends) {
	 if (!(flagl[i] & RPMSENSE_PREREQ))
	     continue;
      }
      
      if (strncmp(namel[i], "rpmlib", 6) == 0) 
      {
#ifdef HAVE_RPM41	
	 rpmds ds = rpmdsSingle(RPMTAG_PROVIDENAME,
			        namel[i], verl?verl[i]:NULL, flagl[i]);
	 int res = rpmCheckRpmlibProvides(ds);
	 rpmdsFree(ds);
#else
	 int res = rpmCheckRpmlibProvides(namel[i], verl?verl[i]:NULL,
					  flagl[i]);
#endif
	 if (res) continue;
      }

      if (verl) 
      {
	 if (!*verl[i]) 
	 {
	    Op = pkgCache::Dep::NoOp;
	 }
	 else 
	 {
	    if (flagl[i] & RPMSENSE_LESS) 
	    {
	       if (flagl[i] & RPMSENSE_EQUAL)
		   Op = pkgCache::Dep::LessEq;
	       else
		   Op = pkgCache::Dep::Less;
	    } 
	    else if (flagl[i] & RPMSENSE_GREATER) 
	    {
	       if (flagl[i] & RPMSENSE_EQUAL)
		   Op = pkgCache::Dep::GreaterEq;
	       else
		   Op = pkgCache::Dep::Greater;
	    } 
	    else if (flagl[i] & RPMSENSE_EQUAL) 
	    {
	       Op = pkgCache::Dep::Equals;
	    }
	 }
	 
	 if (NewDepends(Ver,string(namel[i]),string(verl[i]),Op,Type) == false)
	     return false;
      } 
      else 
      {
	 if (NewDepends(Ver,string(namel[i]),string(),pkgCache::Dep::NoOp,
			Type) == false)
	     return false;
      }
   }
   return true;
}
                                                                        /*}}}*/
// ListParser::ParseDepends - Parse a dependency list			/*{{{*/
// ---------------------------------------------------------------------
/* This is the higher level depends parser. It takes a tag and generates
 a complete depends tree for the given version. */
bool rpmListParser::ParseDepends(pkgCache::VerIterator Ver,
				 unsigned int Type)
{
   char **namel = NULL;
   char **verl = NULL;
   int *flagl = NULL;
   int res, type, count;
   
   switch (Type) 
   {
   case pkgCache::Dep::Depends:
   case pkgCache::Dep::PreDepends:
      res = headerGetEntry(header, RPMTAG_REQUIRENAME, &type, 
			   (void **)&namel, &count);
      if (res != 1)
	  return true;
      res = headerGetEntry(header, RPMTAG_REQUIREVERSION, &type, 
			   (void **)&verl, &count);
      res = headerGetEntry(header, RPMTAG_REQUIREFLAGS, &type,
			   (void **)&flagl, &count);
      break;
      
   case pkgCache::Dep::Obsoletes:
      res = headerGetEntry(header, RPMTAG_OBSOLETENAME, &type,
			   (void **)&namel, &count);
      if (res != 1)
	  return true;
      res = headerGetEntry(header, RPMTAG_OBSOLETEVERSION, &type,
			   (void **)&verl, &count);
      res = headerGetEntry(header, RPMTAG_OBSOLETEFLAGS, &type,
			   (void **)&flagl, &count);      
      break;

   case pkgCache::Dep::Conflicts:
      res = headerGetEntry(header, RPMTAG_CONFLICTNAME, &type, 
			   (void **)&namel, &count);
      if (res != 1)
	  return true;
      res = headerGetEntry(header, RPMTAG_CONFLICTVERSION, &type, 
			   (void **)&verl, &count);
      res = headerGetEntry(header, RPMTAG_CONFLICTFLAGS, &type,
			   (void **)&flagl, &count);
      break;
   }
   
   ParseDepends(Ver, namel, verl, flagl, count, Type);
   
   return true;
}
                                                                        /*}}}*/
#ifdef OLD_FILEDEPS
bool rpmListParser::ProcessFileProvides(pkgCache::VerIterator Ver)
{
   const char **names = NULL;    
   int count = 0;

   rpmHeaderGetEntry(header, RPMTAG_OLDFILENAMES, NULL, &names, &count);

   while (count--) 
   {
      if (rpmSys.IsFileDep(string(names[count]))) 
      {
	 if (!NewProvides(Ver, string(names[count]), string()))
	     return false;
      }
   }

   return true;
}
#endif

bool rpmListParser::CollectFileProvides(pkgCache &Cache,
					pkgCache::VerIterator Ver)
{
   const char **names = NULL;
   int_32 count = 0;
   rpmHeaderGetEntry(header, RPMTAG_OLDFILENAMES,
		     NULL, (void **) &names, &count);
   while (count--) 
   {
      pkgCache::Package *P = Cache.FindPackage(names[count]);
      if (P != NULL && !NewProvides(Ver, string(names[count]), string()))
	 return false;
   }

   return true;
}

// ListParser::ParseProvides - Parse the provides list			/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmListParser::ParseProvides(pkgCache::VerIterator Ver)
{
   int type, count;
   char **namel = NULL;
   char **verl = NULL;
   int res;
   bool ok = true;

   if (Duplicated == true) 
   {
      char *name;
      headerGetEntry(header, RPMTAG_NAME, &type, (void **)&name, &count);
      NewProvides(Ver, string(name), Version());
   }

   res = headerGetEntry(header, RPMTAG_PROVIDENAME, &type,
			(void **)&namel, &count);
   if (res != 1)
       return true;
   /*
    res = headerGetEntry(header, RPMTAG_PROVIDEFLAGS, &type,
    (void **)&flagl, &count);
    if (res != 1)
    return true;
    */
   res = headerGetEntry(header, RPMTAG_PROVIDEVERSION, &type, 
			(void **)&verl, NULL);
   if (res != 1)
	verl = NULL;

   for (int i = 0; i < count; i++) 
   {      
      if (verl && *verl[i]) 
      {
	 if (NewProvides(Ver,string(namel[i]),string(verl[i])) == false) 
	 {
	    ok = false;
	    break;
	 }
      } 
      else 
      {
	 if (NewProvides(Ver,string(namel[i]),string()) == false) 
	 {
	    ok = false;
	    break;
	 }
      }
   }
    
   return ok;
}
                                                                        /*}}}*/
// ListParser::Step - Move to the next section in the file		/*{{{*/
// ---------------------------------------------------------------------
/* This has to be carefull to only process the correct architecture */
bool rpmListParser::Step()
{
   while (Handler->Skip() == true)
   {
      /* See if this is the correct Architecture, if it isn't then we
       drop the whole section. A missing arch tag can't happen to us */
      header = Handler->GetHeader();
      CurrentName = "";
      
      string RealName = Package();
      if (Duplicated == true)
	 RealName = RealName.substr(0,RealName.find('#'));
      if (RpmData->IgnorePackage(RealName) == true)
	 continue;
 
#if OLD_BESTARCH
      bool archOk = false;
      string tmp = rpmSys.BestArchForPackage(RealName);
      if (tmp.empty() == true && // has packages for a single arch only
	  rpmMachineScore(RPM_MACHTABLE_INSTARCH, arch.c_str()) > 0)
	 archOk = true;
      else if (arch == tmp)
	 archOk = true;
      if (Handler->IsDatabase() == true || archOk == true)
	 return true;
#else
      if (Handler->IsDatabase() == true)
	 return true;

      string Arch = Architecture();
      int Score = rpmMachineScore(RPM_MACHTABLE_INSTARCH, Arch.c_str());
      if (Score > 0 && RpmData->AcceptArchScore(Score))
	 return true;
#endif
   }
   header = NULL;
   return false;
}
                                                                        /*}}}*/
// ListParser::LoadReleaseInfo - Load the release information		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool rpmListParser::LoadReleaseInfo(pkgCache::PkgFileIterator FileI,
				    FileFd &File)
{
   pkgTagFile Tags(&File);
   pkgTagSection Section;
   if (!Tags.Step(Section))
       return false;
   
   const char *Start;
   const char *Stop;
   if (Section.Find("Archive",Start,Stop))
       FileI->Archive = WriteUniqString(Start,Stop - Start);
   if (Section.Find("Component",Start,Stop))
       FileI->Component = WriteUniqString(Start,Stop - Start);
   if (Section.Find("Version",Start,Stop))
       FileI->Version = WriteUniqString(Start,Stop - Start);
   if (Section.Find("Origin",Start,Stop))
       FileI->Origin = WriteUniqString(Start,Stop - Start);
   if (Section.Find("Label",Start,Stop))
       FileI->Label = WriteUniqString(Start,Stop - Start);
   if (Section.Find("Architecture",Start,Stop))
       FileI->Architecture = WriteUniqString(Start,Stop - Start);
   
   if (Section.FindFlag("NotAutomatic",FileI->Flags,
			pkgCache::Flag::NotAutomatic) == false)
       _error->Warning("Bad NotAutomatic flag");
   
   return !_error->PendingError();
}
                                                                        /*}}}*/

unsigned long rpmListParser::Size() 
{
   uint_32 *size;
   int type, count;
      
   if (headerGetEntry(header, RPMTAG_SIZE, &type, (void **)&size, &count)!=1)
       return 1;
      
   return (size[0]+512)/1024;
}

// This is a slightly complex operation. It must take a package, and
// move every version to new packages, named accordingly to
// Allow-Duplicated rules.
void rpmListParser::VirtualizePackage(string Name)
{
   pkgCache::PkgIterator FromPkgI = Owner->GetCache().FindPkg(Name);

   // Should always be false
   if (FromPkgI.end() == true)
      return;

   pkgCache::VerIterator FromVerI = FromPkgI.VersionList();
   while (FromVerI.end() == false) {
      string MangledName = Name+"#"+string(FromVerI.VerStr());

      // Get the new package.
      pkgCache::PkgIterator ToPkgI = Owner->GetCache().FindPkg(MangledName);
      if (ToPkgI.end() == true) {
	 // Theoretically, all packages virtualized should pass here at least
	 // once for each new version in the list, since either the package was
	 // already setup as Allow-Duplicated (and this method would never be
	 // called), or the package doesn't exist before getting here. If
	 // we discover that this assumption is false, then we must do
	 // something to order the version list correctly, since the package
	 // could already have some other version there.
	 Owner->NewPackage(ToPkgI, MangledName);

	 // Should it get the flags from the original package? Probably not,
	 // or automatic Allow-Duplicated would work differently than
	 // hardcoded ones.
	 ToPkgI->Flags |= RpmData->PkgFlags(MangledName);
	 ToPkgI->Section = FromPkgI->Section;
      }
      
      // Move the version to the new package.
      FromVerI->ParentPkg = ToPkgI.Index();

      // Put it at the end of the version list (about ordering,
      // read the comment above).
      map_ptrloc *ToVerLast = &ToPkgI->VersionList;
      for (pkgCache::VerIterator ToVerLastI = ToPkgI.VersionList();
	   ToVerLastI.end() == false;
	   ToVerLast = &ToVerLastI->NextVer, ToVerLast++);

      *ToVerLast = FromVerI.Index();

      // Provide the real package name with the current version.
      NewProvides(FromVerI, Name, FromVerI.VerStr());

      // Is this the current version of the old package? If yes, set it
      // as the current version of the new package as well.
      if (FromVerI == FromPkgI.CurrentVer()) {
	 ToPkgI->CurrentVer = FromVerI.Index();
	 ToPkgI->SelectedState = pkgCache::State::Install;
	 ToPkgI->InstState = pkgCache::State::Ok;
	 ToPkgI->CurrentState = pkgCache::State::Installed;
      }

      // Move the iterator before reseting the NextVer.
      pkgCache::Version *FromVer = (pkgCache::Version*)FromVerI;
      FromVerI++;
      FromVer->NextVer = 0;
   }

   // Reset original package data.
   FromPkgI->CurrentVer = 0;
   FromPkgI->VersionList = 0;
   FromPkgI->Section = 0;
   FromPkgI->SelectedState = 0;
   FromPkgI->InstState = 0;
   FromPkgI->CurrentState = 0;
}

#endif /* HAVE_RPM */

// vim:sts=3:sw=3
