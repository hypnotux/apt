// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: rpmpm.h,v 1.4 2003/01/29 13:52:32 niemeyer Exp $
/* ######################################################################

   rpm Package Manager - Provide an interface to rpm
   
   ##################################################################### 
 */
									/*}}}*/
#ifndef PKGLIB_rpmPM_H
#define PKGLIB_rpmPM_H

#ifdef __GNUG__
#pragma interface "apt-pkg/rpmpm.h"
#endif

#include <apt-pkg/packagemanager.h>
#include <vector>
#include <list>

using namespace std;

class pkgRPMPM : public pkgPackageManager
{
   protected:

   struct Item
   {
      enum Ops {Install, Configure, Remove, Purge} Op;
      enum RPMOps {RPMInstall, RPMUpgrade, RPMErase};
      string File;
      PkgIterator Pkg;
      Item(Ops Op,PkgIterator Pkg,string File = "")
	 : Op(Op), File(File), Pkg(Pkg) {};
      Item() {};
      
   };
   vector<Item> List;

   // Helpers
   bool RunScripts(const char *Cnf);
   bool RunScriptsWithPkgs(const char *Cnf);
   
   // The Actuall installation implementation
   virtual bool Install(PkgIterator Pkg,string File);
   virtual bool Configure(PkgIterator Pkg);
   virtual bool Remove(PkgIterator Pkg,bool Purge = false);
    
   bool ExecRPM(Item::RPMOps op, list<const char*> &files);
   bool Process(list<const char*> &install,
		list<const char*> &upgrade,
		list<const char*> &uninstall);
   
   virtual bool Go();
   virtual void Reset();
   
   public:

   pkgRPMPM(pkgDepCache *Cache);
   virtual ~pkgRPMPM();
};

#endif

// vim:sts=3:sw=3
