/*
 * $Id: genpkglist.cc,v 1.7 2003/01/30 17:18:21 niemeyer Exp $
 */
#include <alloca.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <rpm/rpmlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include <map>
#include <iostream>

#include <apt-pkg/error.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/configuration.h>
#include <config.h>

#include "rpmhandler.h"
#include "cached_md5.h"
#include "genutil.h"

#include <rpm/rpmts.h>

#define CRPMTAG_TIMESTAMP   1012345

using namespace std;

raptTag tags[] =  {
       RPMTAG_NAME, 
       RPMTAG_EPOCH,
       RPMTAG_VERSION,
       RPMTAG_RELEASE,
       RPMTAG_GROUP,
       RPMTAG_ARCH,
       RPMTAG_PACKAGER,
       RPMTAG_SOURCERPM,
       RPMTAG_SIZE,
       RPMTAG_VENDOR,
       RPMTAG_OS,
       
       RPMTAG_DESCRIPTION, 
       RPMTAG_SUMMARY, 
       RPMTAG_HEADERI18NTABLE,
       
       RPMTAG_REQUIREFLAGS, 
       RPMTAG_REQUIRENAME,
       RPMTAG_REQUIREVERSION,
       
       RPMTAG_CONFLICTFLAGS,
       RPMTAG_CONFLICTNAME,
       RPMTAG_CONFLICTVERSION,
       
       RPMTAG_PROVIDENAME,
       RPMTAG_PROVIDEFLAGS,
       RPMTAG_PROVIDEVERSION,
       
       RPMTAG_OBSOLETENAME,
       RPMTAG_OBSOLETEFLAGS,
       RPMTAG_OBSOLETEVERSION
};
int numTags = sizeof(tags) / sizeof(raptTag);

/* Can't use headerPutFoo() helpers for custom tags */
static int hdrPut(Header h, raptTag tag, raptTagType type, const void * data)
{
    struct rpmtd_s td = { tag, type, 1, (void *) data, RPMTD_NONE, 0 };
    return headerPut(h, &td, HEADERPUT_DEFAULT);
}

static
int usefulFile(const char *fn)
{
   // PATH-like directories
   if (strstr(fn, "/bin/") || strstr(fn, "/sbin/"))
      return 1;

   // shared libraries
   const char *bn = basename(fn);
   if (bn && strncmp(bn, "lib", 3) == 0 && strstr(bn + 3, ".so"))
      return 1;

   return 0;
}

static void copyStrippedFileList(Header header, Header newHeader)
{
   struct rpmtd_s files;
   if (headerGet(header, RPMTAG_FILENAMES, &files, HEADERGET_EXT)) {
      const char *fn;
      while ((fn = rpmtdNextString(&files)) != NULL) {
          if (usefulFile(fn))
              headerPutString(newHeader, RPMTAG_FILENAMES, fn);
      }
      headerConvert(newHeader, HEADERCONV_COMPRESSFILELIST);
   }
}

bool copyFields(Header h, Header newHeader,
		FILE *idxfile, const char *directory, char *filename,
		unsigned filesize, bool fullFileList)
{
   struct rpmtd_s td;
   int i;
   raptInt size[1];

   size[0] = filesize;
   
   // the std tags
   for (i = 0; i < numTags; i++) {
      if (headerGet(h, tags[i], &td, HEADERGET_RAW)) {
         headerPut(newHeader, &td, HEADERPUT_DEFAULT);
         rpmtdFreeData(&td);
      }
   }
 
   if (fullFileList) {
      struct rpmtd_s dn, bn, di;

      if (headerGet(h, RPMTAG_DIRNAMES, &dn, HEADERGET_MINMEM) &&
            headerGet(h, RPMTAG_BASENAMES, &bn, HEADERGET_MINMEM) &&
            headerGet(h, RPMTAG_DIRINDEXES, &di, HEADERGET_MINMEM)) {
         headerPut(newHeader, &dn, HEADERPUT_DEFAULT);
         headerPut(newHeader, &bn, HEADERPUT_DEFAULT);
         headerPut(newHeader, &di, HEADERPUT_DEFAULT);
      }
   } else {
       copyStrippedFileList(h, newHeader);
   }
   
   // update index of srpms
   if (idxfile) {
      const char *name = headerGetString(h, RPMTAG_NAME);
      const char *srpm = headerGetString(h, RPMTAG_SOURCERPM);

      if (name && srpm) {
	 fprintf(idxfile, "%s %s\n", srpm, name);
      }
   }
   // our additional tags
   hdrPut(newHeader, CRPMTAG_DIRECTORY, RPM_STRING_TYPE, directory);
   hdrPut(newHeader, CRPMTAG_FILENAME, RPM_STRING_TYPE, filename);
   hdrPut(newHeader, CRPMTAG_FILESIZE, RPM_INT32_TYPE, size);
   
   return true;
}


void usage()
{
   cerr << "genpkglist " << VERSION << endl;
   cerr << "usage: genpkglist [<options>] <dir> <suffix>" << endl;
   cerr << "options:" << endl;
   cerr << " --index <file>  file to write srpm index data to" << endl;
   cerr << " --meta <suffix> create package file list with given suffix" << endl;
   cerr << " --bloat         do not strip the package file list. Needed for some" << endl;
   cerr << "                 distributions that use non-automatically generated" << endl;
   cerr << "                 file dependencies" << endl;
   cerr << " --append        append to the package file list, don't overwrite" << endl;
   cerr << " --progress      show a progress bar" << endl;
   cerr << " --cachedir=DIR  use a custom directory for package md5sum cache"<<endl;
}


int main(int argc, char ** argv) 
{
   string rpmsdir;
   string pkglist_path;
   FD_t outfd, fd;
   struct dirent **dirEntries;
   int entry_no, entry_cur;
   CachedMD5 *md5cache;
   char *op_dir;
   char *op_suf;
   char *op_index = NULL;
   FILE *idxfile;
   int i;
   bool fullFileList = false;
   bool progressBar = false;
   const char *pkgListSuffix = NULL;
   bool pkgListAppend = false;
   
   putenv((char *)"LC_ALL="); // Is this necessary yet (after i18n was supported)?
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--index") == 0) {
	 i++;
	 if (i < argc) {
	    op_index = argv[i];
	 } else {
	    cout << "genpkglist: filename missing for option --index"<<endl;
	    exit(1);
	 }
      } else if (strcmp(argv[i], "--bloat") == 0) {
	 fullFileList = true;
      } else if (strcmp(argv[i], "--progress") == 0) {
	 progressBar = true;
      } else if (strcmp(argv[i], "--append") == 0) {
	 pkgListAppend = true;
      } else if (strcmp(argv[i], "--meta") == 0) {
	 i++;
	 if (i < argc) {
	    pkgListSuffix = argv[i];
	 } else {
	    cout << "genpkglist: argument missing for option --meta"<<endl;
	    exit(1);
	 }
      } else if (strcmp(argv[i], "--cachedir") == 0) {
	 i++;
	 if (i < argc) {
            _config->Set("Dir::Cache", argv[i]);
	 } else {
            cout << "genpkglist: argument missing for option --cachedir"<<endl;
	    exit(1);
	 }
      } else {
	 break;
      }
   }
   if (argc - i > 0)
       op_dir = argv[i++];
   else {
      usage();
      exit(1);
   }
   if (argc - i > 0)
       op_suf = argv[i++];
   else {
      usage();
      exit(1);
   }
   if (argc != i) {
      usage();
   }
   
   if (op_index) {
      idxfile = fopen(op_index, "w+");
      if (!idxfile) {
	 cerr << "genpkglist: could not open " << op_index << " for writing";
	 perror("");
	 exit(1);
      }
   } else {
      idxfile = NULL;
   }
   
   {
      char cwd[PATH_MAX];
      
      if (getcwd(cwd, PATH_MAX) == 0)
      {
         cerr << argv[0] << ": " << strerror(errno) << endl;
         exit(1);
      }
      if (*op_dir != '/') {
	 rpmsdir = string(cwd) + "/" + string(op_dir);
      } else {
	 rpmsdir = string(op_dir);
      }
   }
   pkglist_path = string(rpmsdir);
   rpmsdir = rpmsdir + "/RPMS." + string(op_suf);

   string dirtag = "RPMS." + string(op_suf);

   entry_no = scandir(rpmsdir.c_str(), &dirEntries, selectRPMs, alphasort);
   if (entry_no < 0) {
      cerr << "genpkglist: error opening directory " << rpmsdir << ": "
	  << strerror(errno) << endl;
      return 1;
   }
   
   if (chdir(rpmsdir.c_str()) != 0)
   {
      cerr << argv[0] << ": " << strerror(errno) << endl;
      return 1;
   }
   
   if (pkgListSuffix != NULL)
	   pkglist_path = pkglist_path + "/base/pkglist." + pkgListSuffix;
   else
	   pkglist_path = pkglist_path + "/base/pkglist." + op_suf;
   
   
   if (pkgListAppend == true && FileExists(pkglist_path)) {
      outfd = Fopen(pkglist_path.c_str(), "a");
   } else {
      unlink(pkglist_path.c_str());
      outfd = Fopen(pkglist_path.c_str(), "w+");
   }
   if (!outfd) {
      cerr << "genpkglist: error creating file " << pkglist_path << ": "
	  << strerror(errno) << endl;
      return 1;
   }

   md5cache = new CachedMD5(string(op_dir) + string(op_suf), "genpkglist");

   rpmReadConfigFiles(NULL, NULL);
   rpmts ts = rpmtsCreate();
   rpmtsSetVSFlags(ts, (rpmVSFlags_e)-1);

   for (entry_cur = 0; entry_cur < entry_no; entry_cur++) {
      struct stat sb;

      if (progressBar) {
         if (entry_cur)
            printf("\b\b\b\b\b\b\b\b\b\b");
         printf(" %04i/%04i", entry_cur + 1, entry_no);
         fflush(stdout);
      }

      if (stat(dirEntries[entry_cur]->d_name, &sb) < 0) {
	    cerr << "\nWarning: " << strerror(errno) << ": " << 
		    dirEntries[entry_cur]->d_name << endl;
	    continue;
      }

      {
	 Header h;
	 int rc;
	 
	 fd = Fopen(dirEntries[entry_cur]->d_name, "r");

	 if (!fd) {
	    cerr << "\nWarning: " << strerror(errno) << ": " << 
		    dirEntries[entry_cur]->d_name << endl;
	    continue;
	 }
	 
	 rc = rpmReadPackageFile(ts, fd, dirEntries[entry_cur]->d_name, &h);
	 if (rc == RPMRC_OK || rc == RPMRC_NOTTRUSTED || rc == RPMRC_NOKEY) {
	    Header newHeader;
	    char md5[34];
	    
	    newHeader = headerNew();
	    
	    copyFields(h, newHeader, idxfile, dirtag.c_str(),
		       dirEntries[entry_cur]->d_name,
		       sb.st_size, fullFileList);

	    md5cache->MD5ForFile(string(dirEntries[entry_cur]->d_name), 
				 sb.st_mtime, md5);
	    hdrPut(newHeader, CRPMTAG_MD5, RPM_STRING_TYPE, md5);

	    headerWrite(outfd, newHeader, HEADER_MAGIC_YES);
	    
	    headerFree(newHeader);
	    headerFree(h);
	 } else {
	    cerr << "\nWarning: Skipping malformed RPM: " << 
		    dirEntries[entry_cur]->d_name << endl;
	 }
	 Fclose(fd);
      }
   }

   Fclose(outfd);

   ts = rpmtsFree(ts);
   
   delete md5cache;

   return 0;
}
