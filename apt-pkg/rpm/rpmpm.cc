// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   RPM Package Manager - Provide an interface to rpm
   
   ##################################################################### 
 */
									/*}}}*/
// Includes								/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/rpmpm.h"
#endif

#include <config.h>

#ifdef HAVE_RPM

#include <apt-pkg/rpmpm.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/luaiface.h>

#include <apti18n.h>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>


#include <rpm/rpmlib.h>
									/*}}}*/

// RPMPM::pkgRPMPM - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgRPMPM::pkgRPMPM(pkgDepCache *Cache) : pkgPackageManager(Cache)
{
}
									/*}}}*/
// RPMPM::pkgRPMPM - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgRPMPM::~pkgRPMPM()
{
}
									/*}}}*/
// RPMPM::Install - Install a package					/*{{{*/
// ---------------------------------------------------------------------
/* Add an install operation to the sequence list */
bool pkgRPMPM::Install(PkgIterator Pkg,string File)
{
   if (File.empty() == true || Pkg.end() == true)
      return _error->Error(_("Internal Error, No file name for %s"),Pkg.Name());

   List.push_back(Item(Item::Install,Pkg,File));
   return true;
}
									/*}}}*/
// RPMPM::Configure - Configure a package				/*{{{*/
// ---------------------------------------------------------------------
/* Add a configure operation to the sequence list */
bool pkgRPMPM::Configure(PkgIterator Pkg)
{
   if (Pkg.end() == true) {
      return false;
   }
   
   List.push_back(Item(Item::Configure,Pkg));
   return true;
}
									/*}}}*/
// RPMPM::Remove - Remove a package					/*{{{*/
// ---------------------------------------------------------------------
/* Add a remove operation to the sequence list */
bool pkgRPMPM::Remove(PkgIterator Pkg,bool Purge)
{
   if (Pkg.end() == true)
      return false;
   
   if (Purge == true)
      List.push_back(Item(Item::Purge,Pkg));
   else
      List.push_back(Item(Item::Remove,Pkg));
   return true;
}
									/*}}}*/
// RPMPM::RunScripts - Run a set of scripts				/*{{{*/
// ---------------------------------------------------------------------
/* This looks for a list of script sto run from the configuration file,
   each one is run with system from a forked child. */
bool pkgRPMPM::RunScripts(const char *Cnf)
{
   Configuration::Item const *Opts = _config->Tree(Cnf);
   if (Opts == 0 || Opts->Child == 0)
      return true;
   Opts = Opts->Child;

   bool error = false;
   for (; Opts != 0; Opts = Opts->Next)
   {
      if (Opts->Value.empty() == true)
         continue;
		
      // Purified Fork for running the script
      pid_t Process = ExecFork();      
      if (Process == 0)
      {
	 if (chdir("/tmp") != 0)
	    _exit(100);

	 const char *Args[4];
	 Args[0] = "/bin/sh";
	 Args[1] = "-c";
	 Args[2] = Opts->Value.c_str();
	 Args[3] = 0;
	 execv(Args[0],(char **)Args);
	 _exit(100);
      }
      
      // Clean up the sub process
      if (ExecWait(Process,Opts->Value.c_str()) == false) {
	 _error->Error(_("Problem executing scripts %s '%s'"),Cnf,
		       Opts->Value.c_str());
	 error = true;
      }
   }
 
   // Restore sig int/quit
   signal(SIGQUIT,SIG_DFL);
   signal(SIGINT,SIG_DFL);   

   if (error)
      return _error->Error(_("Sub-process returned an error code"));
   
   return true;
}

                                                                        /*}}}*/
// RPMPM::RunScriptsWithPkgs - Run scripts with package names on stdin /*{{{*/
// ---------------------------------------------------------------------
/* This looks for a list of scripts to run from the configuration file
   each one is run and is fed on standard input a list of all .deb files
   that are due to be installed. */
bool pkgRPMPM::RunScriptsWithPkgs(const char *Cnf)
{
   Configuration::Item const *Opts = _config->Tree(Cnf);
   if (Opts == 0 || Opts->Child == 0)
      return true;
   Opts = Opts->Child;
   
   for (; Opts != 0; Opts = Opts->Next)
   {
      if (Opts->Value.empty() == true)
         continue;
		
      // Create the pipes
      int Pipes[2];
      if (pipe(Pipes) != 0)
	 return _error->Errno("pipe","Failed to create IPC pipe to subprocess");
      SetCloseExec(Pipes[0],true);
      SetCloseExec(Pipes[1],true);
      
      // Purified Fork for running the script
      pid_t Process = ExecFork();      
      if (Process == 0)
      {
	 // Setup the FDs
	 dup2(Pipes[0],STDIN_FILENO);
	 SetCloseExec(STDOUT_FILENO,false);
	 SetCloseExec(STDIN_FILENO,false);      
	 SetCloseExec(STDERR_FILENO,false);

	 const char *Args[4];
	 Args[0] = "/bin/sh";
	 Args[1] = "-c";
	 Args[2] = Opts->Value.c_str();
	 Args[3] = 0;
	 execv(Args[0],(char **)Args);
	 _exit(100);
      }
      close(Pipes[0]);
      FileFd Fd(Pipes[1]);

      // Feed it the filenames.
      for (vector<Item>::iterator I = List.begin(); I != List.end(); I++)
      {
	 // Only deal with packages to be installed from .rpm
	 if (I->Op != Item::Install)
	    continue;

	 // No errors here..
	 if (I->File[0] != '/')
	    continue;
	 
	 /* Feed the filename of each package that is pending install
	    into the pipe. */
	 if (Fd.Write(I->File.c_str(),I->File.length()) == false || 
	     Fd.Write("\n",1) == false)
	 {
	    kill(Process,SIGINT);	    
	    Fd.Close();   
	    ExecWait(Process,Opts->Value.c_str(),true);
	    return _error->Error(_("Failure running script %s"),Opts->Value.c_str());
	 }
      }
      Fd.Close();
      
      // Clean up the sub process
      if (ExecWait(Process,Opts->Value.c_str()) == false)
	 return _error->Error(_("Failure running script %s"),Opts->Value.c_str());
   }

   return true;
}

									/*}}}*/

bool pkgRPMPM::ExecRPM(Item::RPMOps op, vector<const char*> &files)
{
   const char *Args[10000];      
   const char *operation;
   unsigned int n = 0;
   bool Interactive = _config->FindB("RPM::Interactive",true);
   
   Args[n++] = _config->Find("Dir::Bin::rpm","rpm").c_str();

   bool nodeps;

   switch (op)
   {
      case Item::RPMInstall:
	 if (Interactive)
	    operation = "-ivh";
	 else
	    operation = "-iv";
	 nodeps = true;
	 break;

      case Item::RPMUpgrade:
	 if (Interactive)
	    operation = "-Uvh";
	 else
	    operation = "-Uv";
	 break;

      case Item::RPMErase:
	 operation = "-e";
	 nodeps = true;
	 break;
   }
   Args[n++] = operation;

   if (Interactive == false && op != Item::RPMErase)
      Args[n++] = "--percent";
    
   string rootdir = _config->Find("RPM::RootDir", "");
   if (!rootdir.empty()) 
   {
       Args[n++] = "-r";
       Args[n++] = rootdir.c_str();
   }

   Configuration::Item const *Opts;
   if (op == Item::RPMErase)
   {
      Opts = _config->Tree("RPM::Erase-Options");
      if (Opts != 0)
      {
	 Opts = Opts->Child;
	 for (; Opts != 0; Opts = Opts->Next)
	 {
	    if (Opts->Value == "--nodeps")
	       nodeps = false;
	    else if (Opts->Value.empty() == true)
	       continue;
	    Args[n++] = Opts->Value.c_str();
	 }
      }
   }
   else
   {
      bool oldpackage = _config->FindB("RPM::OldPackage",
				       (op == Item::RPMUpgrade));
      bool replacepkgs = _config->FindB("APT::Get::ReInstall",false);
      bool replacefiles = _config->FindB("APT::Get::ReInstall",false);
      Opts = _config->Tree("RPM::Install-Options");
      if (Opts != 0)
      {
	 Opts = Opts->Child;
	 for (; Opts != 0; Opts = Opts->Next)
	 {
	    if (Opts->Value == "--oldpackage")
	       oldpackage = false;
	    else if (Opts->Value == "--replacepkgs")
	       replacepkgs = false;
	    else if (Opts->Value == "--replacefiles")
	       replacefiles = false;
	    else if (Opts->Value == "--nodeps")
	       nodeps = false;
	    else if (Opts->Value.empty() == true)
	       continue;
	    Args[n++] = Opts->Value.c_str();
	 }	 
      }
      if (oldpackage == true)
	 Args[n++] = "--oldpackage";
      if (replacepkgs == true)
	 Args[n++] = "--replacepkgs";
      if (replacefiles == true)
	 Args[n++] = "--replacefiles";
   }

   if (nodeps == true)
      Args[n++] = "--nodeps";

   Opts = _config->Tree("RPM::Options");
   if (Opts != 0)
   {
      Opts = Opts->Child;
      for (; Opts != 0; Opts = Opts->Next)
      {
	 if (Opts->Value.empty() == true)
	    continue;
	 Args[n++] = Opts->Value.c_str();
      }	 
   }

   if (_config->FindB("RPM::Order",false) == false)
      Args[n++] = "--noorder";
    
   bool FilesInArgs = true;
   char *ArgsFileName = NULL;
#ifdef HAVE_RPM4
   if (op != Item::RPMErase && files.size() > 50) {
      string FileName = _config->FindDir("Dir::Cache", "/tmp/") +
			"filelist.XXXXXX";
      ArgsFileName = strdup(FileName.c_str());
      if (ArgsFileName) {
	 int fd = mkstemp(ArgsFileName);
	 if (fd != -1) {
	    FileFd File(fd);
	    for (vector<const char*>::iterator I = files.begin();
		 I != files.end(); I++) {
	       File.Write(*I, strlen(*I));
	       File.Write("\n", 1);
	    }
	    File.Close();
	    FilesInArgs = false;
	    Args[n++] = ArgsFileName;
	 }
      }
   }
#endif

   if (FilesInArgs == true) {
      for (vector<const char*>::iterator I = files.begin();
	   I != files.end(); I++)
	 Args[n++] = *I;
   }
   
   Args[n++] = 0;

   if (_config->FindB("Debug::pkgRPMPM",false) == true)
   {
      for (unsigned int k = 0; k < n; k++)
	  clog << Args[k] << ' ';
      clog << endl;
      if (ArgsFileName) {
	 unlink(ArgsFileName);
	 free(ArgsFileName);
      }
      return true;
   }

   cout << _("Executing RPM (")<<operation<<")..." << endl;

   cout << flush;
   clog << flush;
   cerr << flush;

   /* Mask off sig int/quit. We do this because dpkg also does when 
    it forks scripts. What happens is that when you hit ctrl-c it sends
    it to all processes in the group. Since dpkg ignores the signal 
    it doesn't die but we do! So we must also ignore it */
   //akk ??
   signal(SIGQUIT,SIG_IGN);
   signal(SIGINT,SIG_IGN);

   // Fork rpm
   pid_t Child = ExecFork();
            
   // This is the child
   if (Child == 0)
   {
      if (chdir(_config->FindDir("RPM::Run-Directory","/").c_str()) != 0)
	  _exit(100);
	 
      if (_config->FindB("RPM::FlushSTDIN",true) == true)
      {
	 int Flags,dummy;
	 if ((Flags = fcntl(STDIN_FILENO,F_GETFL,dummy)) < 0)
	     _exit(100);
	 
	 // Discard everything in stdin before forking dpkg
	 if (fcntl(STDIN_FILENO,F_SETFL,Flags | O_NONBLOCK) < 0)
	     _exit(100);
	 
	 while (read(STDIN_FILENO,&dummy,1) == 1);
	 
	 if (fcntl(STDIN_FILENO,F_SETFL,Flags & (~(long)O_NONBLOCK)) < 0)
	     _exit(100);
      }

      execvp(Args[0],(char **)Args);
      cerr << "Could not exec " << Args[0] << endl;
      _exit(100);
   }      
   
   // Wait for rpm
   int Status = 0;
   while (waitpid(Child,&Status,0) != Child)
   {
      if (errno == EINTR)
	  continue;
      RunScripts("RPM::Post-Invoke");
      if (ArgsFileName) {
	 unlink(ArgsFileName);
	 free(ArgsFileName);
      }
      return _error->Errno("waitpid","Couldn't wait for subprocess");
   }
   if (ArgsFileName) {
      unlink(ArgsFileName);
      free(ArgsFileName);
   }

   // Restore sig int/quit
   signal(SIGQUIT,SIG_DFL);
   signal(SIGINT,SIG_DFL);
       
   // Check for an error code.
   if (WIFEXITED(Status) == 0 || WEXITSTATUS(Status) != 0)
   {
      RunScripts("RPM::Post-Invoke");
      if (WIFSIGNALED(Status) != 0 && WTERMSIG(Status) == SIGSEGV)
	  return _error->Error(_("Sub-process %s recieved a segmentation fault."),Args[0]);
      
      if (WIFEXITED(Status) != 0)
	  return _error->Error(_("Sub-process %s returned an error code (%u)"),Args[0],
			       WEXITSTATUS(Status));
      
      return _error->Error(_("Sub-process %s exited unexpectedly"),Args[0]);
   }

   return true;
}


bool pkgRPMPM::Process(vector<const char*> &install, 
		       vector<const char*> &upgrade,
		       vector<const char*> &uninstall)
{
   if (uninstall.empty() == false)
       ExecRPM(Item::RPMErase, uninstall);
   if (install.empty() == false)
       ExecRPM(Item::RPMInstall, install);
   if (upgrade.empty() == false)
       ExecRPM(Item::RPMUpgrade, upgrade);
   return true;
}


// RPMPM::Go - Run the sequence					/*{{{*/
// ---------------------------------------------------------------------
/* This globs the operations and calls rpm */
bool pkgRPMPM::Go()
{
   if (RunScripts("RPM::Pre-Invoke") == false)
      return false;

   if (RunScriptsWithPkgs("RPM::Pre-Install-Pkgs") == false)
      return false;
   
   vector<const char*> install_or_upgrade;
   vector<const char*> install;
   vector<const char*> upgrade;
   vector<const char*> uninstall;
   vector<pkgCache::Package*> pkgs_install;
   vector<pkgCache::Package*> pkgs_uninstall;

   vector<char*> unalloc;
   
   for (vector<Item>::iterator I = List.begin(); I != List.end(); I++)
   {
      switch (I->Op)
      {
      case Item::Purge:
      case Item::Remove:
	 if (strchr(I->Pkg.Name(), '#') != NULL)
	 {
	    char *name = strdup(I->Pkg.Name());
	    char *p = strchr(name, '#');
	    *(p++) = '-';
	    const char *epoch = strchr(p, ':');
	    if (epoch != NULL)
	       memmove(p, epoch+1, strlen(epoch+1)+1);
	    unalloc.push_back(name);
	    uninstall.push_back(name);
	 }
	 else
	    uninstall.push_back(I->Pkg.Name());
	 pkgs_uninstall.push_back(I->Pkg);
	 break;

       case Item::Configure:
	 break;

       case Item::Install:
	 if (strchr(I->Pkg.Name(), '#') != NULL)
	    install.push_back(I->File.c_str());
	 else
	    upgrade.push_back(I->File.c_str());
	 install_or_upgrade.push_back(I->File.c_str());
	 pkgs_install.push_back(I->Pkg);
	 break;
	  
       default:
	 return _error->Error("Unknown pkgRPMPM operation.");
      }
   }

   bool Ret = true;

#ifdef WITH_LUA
   if (_lua->HasScripts("Scripts::RPM::Pre") == true) {
      _lua->SetGlobal("files_install", install_or_upgrade);
      _lua->SetGlobal("names_remove", uninstall);
      _lua->SetGlobal("pkgs_install", pkgs_install);
      _lua->SetGlobal("pkgs_remove", pkgs_uninstall);
      _lua->SetDepCache(&Cache);
      _lua->RunScripts("Scripts::RPM::Pre", false);
      _lua->ResetCaches();
      _lua->ResetGlobals();
      if (_error->PendingError() == true) {
	 Ret = false;
	 goto exit;
      }
   }
#endif

   if (Process(install, upgrade, uninstall) == false)
      Ret = false;

#ifdef WITH_LUA
   if (_lua->HasScripts("Scripts::RPM::Post") == true) {
      _lua->SetGlobal("files_install", install_or_upgrade);
      _lua->SetGlobal("names_remove", uninstall);
      _lua->SetGlobal("pkgs_install", pkgs_install);
      _lua->SetGlobal("pkgs_remove", pkgs_uninstall);
      _lua->SetDepCache(&Cache);
      _lua->RunScripts("Scripts::RPM::Post", false);
      _lua->ResetCaches();
      _lua->ResetGlobals();
      if (_error->PendingError() == true) {
	 Ret = false;
	 goto exit;
      }
   }
#endif

   
   if (Ret == true)
      Ret = RunScripts("RPM::Post-Invoke");

exit:
   for (vector<char *>::iterator I = unalloc.begin(); I != unalloc.end(); I++)
      free(*I);

   return Ret;
}
									/*}}}*/
// pkgRPMPM::Reset - Dump the contents of the command list		/*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgRPMPM::Reset() 
{
   List.erase(List.begin(),List.end());
}
									/*}}}*/

#endif /* HAVE_RPM */

// vim:sts=3:sw=3
