/*
 * $Id: hdlist2pkglist.cc,v 1.4 2002/07/26 23:16:34 niemeyer Exp $
 */
#include <alloca.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <rpmlib.h>
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
#include <apt-pkg/rpmhandler.h>
#include <apt-pkg/md5.h>

#include <config.h>

using namespace std;

#define CRPMTAG_TIMESTAMP   1012345

int tags[] =  {
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
   
   RPMTAG_DESCRIPTION, 
   RPMTAG_SUMMARY, 
   
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

int numTags = sizeof(tags) / sizeof(int);

typedef struct {
   string importance;
   string date;
   string summary;
   string url;
} UpdateInfo;

static inline int usefullFile(char *a)
{
   int l = strlen(a);
   if (strstr(a, "bin"))
      return 1;
   if (l < 3)
      return 0;
   if (strcmp(a + l - 2, ".a") == 0
       || strcmp(a + l - 3, ".so") == 0
       || strstr(a, ".so."))
      return 1;
   return 0;
}

static void copyStrippedFileList(Header header, Header newHeader)
{
   int i;
   int i1, i2;
   
   int type1, type2, type3;
   int count1, count2, count3;
   char **dirnames = NULL, **basenames = NULL;
   int_32 *dirindexes = NULL;
   char **dnames, **bnames;
   int_32 *dindexes;
   int res1, res2, res3;
   
   res1 = headerGetEntry(header, RPMTAG_DIRNAMES, &type1, 
			 (void**)&dirnames, &count1);
   res2 = headerGetEntry(header, RPMTAG_BASENAMES, &type2, 
			 (void**)&basenames, &count2);
   res3 = headerGetEntry(header, RPMTAG_DIRINDEXES, &type3, 
			 (void**)&dirindexes, &count3);
   
   if (res1 != 1 || res2 != 1 || res3 != 1)
   {
      free(dirnames);
      free(basenames);
      return;
   }
   
   dnames = dirnames;
   bnames = basenames;
   dindexes = (int_32*)malloc(sizeof(int_32)*count3);
   
   i1 = 0;
   i2 = 0;
   for (i = 0; i < count2 ; i++) 
   {
      int ok = 0;
      
      ok = usefullFile(basenames[i]);
      if (!ok) 
	  ok = usefullFile(dirnames[dirindexes[i]]);
      
      if (!ok)
      {
	 int k = i;
	 while (dirindexes[i] == dirindexes[k] && i < count2)
	     i++;
	 i--;
	 continue;
      }
      
      
      if (ok)
      {
	 int j;
	 
	 bnames[i1] = basenames[i];
	 for (j = 0; j < i2; j++)
	 {
	    if (dnames[j] == dirnames[dirindexes[i]])
	    {
	       dindexes[i1] = j;
	       break;
	    }
	 }
	 if (j == i2) 
	 {
	    dnames[i2] = dirnames[dirindexes[i]];
	    dindexes[i1] = i2;
	    i2++;
	 }
	 assert(i2 <= count1);
	 i1++;
      } 
   }
   
   if (i1 == 0)
   {
      free(dirnames);
      free(basenames);
      free(dindexes);
      return;
   }
   
   headerAddEntry(newHeader, RPMTAG_DIRNAMES, type1, dnames, i2);
   headerAddEntry(newHeader, RPMTAG_BASENAMES, type2, bnames, i1);
   headerAddEntry(newHeader, RPMTAG_DIRINDEXES, type3, dindexes, i1);
   
   free(dirnames);
   free(basenames);
   free(dindexes);
}





bool loadUpdateInfo(char *path, map<string,UpdateInfo> &map)
{
   FileFd F(path, FileFd::ReadOnly);
   if (_error->PendingError()) 
   {
      return false;
   }
   
   pkgTagFile Tags(&F);
   pkgTagSection Section;
   
   while (Tags.Step(Section)) 
   {
      string file = Section.FindS("File");
      UpdateInfo info;
      
      info.importance = Section.FindS("Importance");
      info.date = Section.FindS("Date");
      info.summary = Section.FindS("Summary");
      info.url = Section.FindS("URL");
      
      map[file] = info;
   }
   return true;
}



bool copyFields(Header h, Header newHeader,
		FILE *idxfile, char *filename, unsigned filesize,
		map<string,UpdateInfo> &updateInfo)
{
   int i;
   int_32 size[1];
   
   size[0] = filesize;
   
   // the std tags
   for (i = 0; i < numTags; i++) {
      int_32 type, count;
      void *data;
      int res;
      
      res = headerGetEntry(h, tags[i], &type, &data, &count);
      if (res != 1)
	 continue;
      headerAddEntry(newHeader, tags[i], type, data, count);
   }
   
   copyStrippedFileList(h, newHeader);
   
   // update index of srpms
   if (idxfile) {
      int_32 type, count;
      char *srpm;
      char *name;
      int res;
      
      res = headerGetEntry(h, RPMTAG_NAME, &type, 
			   (void**)&name, &count);
      res = headerGetEntry(h, RPMTAG_SOURCERPM, &type, 
			   (void**)&srpm, &count);
      if (res == 1) {
	 fprintf(idxfile, "%s %s\n", srpm, name);
      }
   }
   // our additional tags
   headerAddEntry(newHeader, CRPMTAG_FILENAME, RPM_STRING_TYPE, 
		  filename, 1);
   headerAddEntry(newHeader, CRPMTAG_FILESIZE, RPM_INT32_TYPE,
		  size, 1);
   
   FileFd File(filename, FileFd::ReadOnly);
   MD5Summation MD5;
   MD5.AddFD(File.Fd(), File.Size());
   File.Close();
   char md5[34];
   strcpy(md5, MD5.Result().Value().c_str());
   headerAddEntry(newHeader, CRPMTAG_MD5, RPM_STRING_TYPE, md5, 1);
   
   // update description tags
   if (updateInfo.find(string(filename)) != updateInfo.end()) {
      const char *tmp;
      string name = string(filename);
      
      tmp = updateInfo[name].summary.c_str();
      headerAddEntry(newHeader, CRPMTAG_UPDATE_SUMMARY,
		     RPM_STRING_TYPE,
		     tmp, 1);
      tmp = updateInfo[name].url.c_str();
      headerAddEntry(newHeader, CRPMTAG_UPDATE_URL,
		     RPM_STRING_TYPE,
		     tmp, 1);
      tmp = updateInfo[name].date.c_str();
      headerAddEntry(newHeader, CRPMTAG_UPDATE_DATE,
		     RPM_STRING_TYPE,
		     tmp, 1);
      tmp = updateInfo[name].importance.c_str();
      headerAddEntry(newHeader, CRPMTAG_UPDATE_IMPORTANCE,
		     RPM_STRING_TYPE,
		     tmp, 1);
   }
   return true;
}


void usage()
{
   cerr << "usage: hdlist2pkglist [<options>] <dir> <suffix>" << endl;
   cerr << "options:" << endl;
   cerr << " --index <file>   file to write srpm index data to" << endl;
   cerr << " --info <file>    file to read update info from" << endl;
}


int main(int argc, char ** argv) 
{
   FD_t outfd;
   FD_t hdlist;
   map<string,UpdateInfo> updateInfo;
   char *op_dir;
   char *op_hdlist;
   char *op_pkglist;
   char *op_update = NULL;
   char *op_index = NULL;

   FILE *idxfile;
   int i;
   
   putenv("LC_ALL=");
   
   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "--index") == 0) {
	 i++;
	 if (i < argc) {
	    op_index = argv[i];
	 } else {
	    cout << "hdlist2pkglist: filename missing for option --index"<<endl;
	    exit(1);
	 }
      } else if (strcmp(argv[i], "--info") == 0) {
	 i++;
	 if (i < argc) {
	    op_update = argv[i];
	 } else {
	    cout << "hdlist2pkglist: filename missing for option --info"<<endl;
	    exit(1);
	 }
      } else {
	 break;
      }
   }
   {
      char *dir, *suffix;
      char buf[512];
      
      if (argc - i > 0)
	  dir = argv[i++];
      else {
	 usage();
	 exit(1);
      }
      if (argc - i > 0)
	  suffix = argv[i++];
      else {
	 usage();
	 exit(1);
      }
      
      sprintf(buf, "%s/RPMS.%s", dir, suffix);
      op_dir = strdup(buf);

      sprintf(buf, "%s/base/hdlist.%s", dir, suffix);
      op_hdlist = strdup(buf);

      sprintf(buf, "%s/base/pkglist.%s", dir, suffix);
      op_pkglist = strdup(buf);
   }

   if (argc != i) {
      usage();
   }
   
   if (op_update) {
      if (!loadUpdateInfo(op_update, updateInfo)) {
	 cerr << "hdlist2pkglist: error reading update info from file " << op_update << endl;
	 _error->DumpErrors();
	 exit(1);
      }
   }
   if (op_index) {
      idxfile = fopen(op_index, "w+");
      if (!idxfile) {
	 cerr << "hdlist2pkglist: could not open " << op_index << " for writing";
	 perror("");
	 exit(1);
      }
   } else {
      idxfile = NULL;
   }
   
   hdlist = fdOpen(op_hdlist, O_RDONLY, 0644);
   if (!hdlist) {
      cerr << "hdlist2pkglist: error opening file " << op_hdlist << ": "
	  << strerror(errno) << endl;
      return 1;
   }
   
   unlink(op_pkglist);
   
   outfd = fdOpen(op_pkglist, O_WRONLY | O_TRUNC | O_CREAT, 0644);
   if (!outfd) {
      cerr << "hdlist2pkglist: error creating file " << op_pkglist << ": "
	  << strerror(errno) << endl;;
      return 1;
   }

   chdir(op_dir);
   

   while (1) {
      struct stat sb;
      Header h;
      Header newHeader;
      char *package;
      int type, count;

      h = headerRead(hdlist, HEADER_MAGIC_YES);
      if (!h)
	  break;

      if (headerGetEntry(h, CRPMTAG_FILENAME, &type, (void**)&package, &count)!=1) {
	 cerr << "hdlist2pkglist: could not get CRPMTAG_FILENAME from header in hdlist" << endl;
	 exit(1);
      }

      if (stat(package, &sb) < 0) {
	 cerr << package << ":" << strerror(errno);
	 exit(1);
      }

	    
      newHeader = headerNew();
	    
      copyFields(h, newHeader, idxfile, package, sb.st_size, updateInfo);

      headerWrite(outfd, newHeader, HEADER_MAGIC_YES);

      headerFree(newHeader);
      headerFree(h);
   } 

   fdClose(hdlist);
   fdClose(outfd);

   return 0;
}

// vim:sts=3:sw=3
