
#ifndef _RPMPACKAGEDATA_H_
#define _RPMPACKAGEDATA_H_


#include <apt-pkg/tagfile.h>
#include <apt-pkg/pkgcache.h>

#include <map>
#include <list>
#include <regex.h>

using namespace std;

class RPMPackageData 
{
   protected:

   map<string,pkgCache::State::VerPriority> Priorities;
   map<string,pkgCache::Flag::PkgFlags> Flags;
   map<string,list<string>*> FakeProvides;
   map<string,int> IgnorePackages;
   list<regex_t*> HoldPackages;   

   struct Translate {
	   regex_t Pattern;
	   string Template;
   };
   
   list<Translate*> BinaryTranslations;
   list<Translate*> SourceTranslations;
   list<Translate*> IndexTranslations;

   void GenericTranslate(list<Translate*> &TList, string &FullURI,
		   	 map<string,string> &Dict);

   int MinArchScore;

   public:

   inline pkgCache::State::VerPriority VerPriority(string Package) 
   	{return Priorities[Package];};
   inline pkgCache::Flag::PkgFlags PkgFlags(string Package) 
   	{return Flags[Package];};

   bool HoldPackage(const char *name);
   bool IgnorePackage(string Name);
   bool IgnoreDep(pkgVersioningSystem &VS,pkgCache::DepIterator &Dep);

   static RPMPackageData *Singleton();

   void TranslateBinary(string &FullURI, map<string,string> &Dict)
   	{return GenericTranslate(BinaryTranslations, FullURI, Dict);};
   void TranslateSource(string &FullURI, map<string,string> &Dict)
   	{return GenericTranslate(SourceTranslations, FullURI, Dict);};
   void TranslateIndex(string &FullURI, map<string,string> &Dict)
   	{return GenericTranslate(IndexTranslations, FullURI, Dict);};

   bool HasBinaryTranslation()
	{return !BinaryTranslations.empty();};
   bool HasSourceTranslation()
	{return !SourceTranslations.empty();};
   bool HasIndexTranslation()
	{return !IndexTranslations.empty();};

   void InitMinArchScore();
   bool AcceptArchScore(int Score) { return Score >= MinArchScore; }

   RPMPackageData();
};


#endif
