// CNC:2002-07-03

#ifndef PKGLIB_REPOSITORY_H
#define PKGLIB_REPOSITORY_H

#include <string>
#include <map>

#include <apt-pkg/sourcelist.h>

using std::map;

class pkgRepository
{
   protected:
   

   struct Checksum {
      unsigned long Size;
      string Hash;
      string HashType;
   };
   
   map<string,Checksum> IndexChecksums; // path -> checksum data   

   bool GotRelease;

   public:   

   string URI;
   string Dist;
   vector<string> FingerPrintList;
   string RootURI;

   bool Acquire;
   
   // LORG:2006-02-21 make these methods virtual
   virtual bool ParseRelease(string File);
   virtual bool HasRelease() const { return GotRelease; }

   virtual bool IsAuthenticated() const { return !FingerPrintList.empty(); }
   virtual bool FindChecksums(string URI,off_t &Size, string &Hash, string &HashType);
   // Only used in repomd atm
   virtual string FindURI(string DataType) {return "";}
   virtual string GetComprMethod(string URI) {return "bz2";}
   
   pkgRepository(string URI,string Dist, const pkgSourceList::Vendor *Vendor,
		 string RootURI)
      : GotRelease(0), URI(URI), Dist(Dist), RootURI(RootURI),
   	Acquire(1)
   {
      if (Vendor) FingerPrintList = Vendor->FingerPrintList;
   }

   virtual ~pkgRepository() {}
};

#endif

// vim:sts=3:sw=3
