#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <rpm/rpmlib.h>

#include "hashtable.h"


HashTable *table;

typedef struct llist {
   struct llist *next;
   char *s;
} llist;


llist *lappend(llist *l, char *s)
{
   llist *b;
   
   b = malloc(sizeof(llist));
   assert(b!=NULL);
   
   b->s = (char*)strdup(s);
   b->next = l;

   return b;
}



#define CRPMTAG_FILENAME 1000000
#define CRPMTAG_FILESIZE 1000001



void pstrarray(FILE *f, char **str, int count)
{
   if (!count)
     return;

   fprintf(f, "%s", *str++);
   count--;
   while (count--) {
      fprintf(f, ", %s", *str++);
   }
   fprintf(f, "\n");
}

void pstr(FILE *f, char **data, int count, int type) 
{
   if (type == RPM_STRING_TYPE) {
      fprintf(f, "%s\n", (char*)data);
   } else if (type == RPM_STRING_ARRAY_TYPE) {
      pstrarray(f, data, count);
   } else {
      puts("Oh shit!");
      abort();
   }
}


void pitem(FILE *f, char *file, char *version, int flags)
{
   fputs(file, f);
   if (*version) {
      int c = 0;
      fputs(" (", f);
      /*
       * For some reason, debian ppl decided that:
       * > and < are the same as >=, <=
       * >> and << is the same as >, <
       */
      if (flags & RPMSENSE_LESS) {
	 fputc('<', f);
	 c = '<';
      }
      if (flags & RPMSENSE_GREATER) {
	 fputc('>', f);
	 c = '>';
      }
      if (flags & RPMSENSE_EQUAL) {
	 fputc('=', f);
      } else {
	 if (c)
	   fputc(c, f);
      }
      fprintf(f, " %s)", version);
   }
}


void ptuplearray(FILE *f, char **str1, char **str2, int *flags, int count)
{
   int first = 1;
   if (!count)
     return;

   while (count--) {      
      if (1||strcmp(*str1, "rpmlib(VersionedDependencies)")!=0) {
	 if (!first)
	   fputs(", ", f);
	 first = 0;
	 pitem(f, *str1, *str2, *flags);
      } else {
	 puts("ignoring: rpmlib(VersionedDependencies)");
      }
      str1++; str2++; flags++;
   }
   fputc('\n', f);
}

void pdepend(FILE *f, char **data1, char **data2, int *flags, int count, 
	    int type)
{
   if (type == RPM_STRING_TYPE) {
      pitem(f, (char*)data1, (char*)data2, *flags);
      fprintf(f, "\n");
   } else if (type == RPM_STRING_ARRAY_TYPE) {
      ptuplearray(f, data1, data2, flags, count);
   } else {
      puts("Oh shit!");
      abort();
   }
}


void dumpDescription(FILE *f, char *descr)
{
   int nl;
   
   nl = 1;
   while (*descr) {
      if (nl) {
	 fputc(' ', f);
	 nl = 0;
      }
      if (*descr=='\n') {
	 nl = 1;
      }
      fputc(*descr++, f);
   }
   fputc('\n', f);
}



void showHeader(FILE *f, Header hdr, char *dir)
{
   int type, type2, type3, count;
   char *str;
   char **strv;
   char **strv2;
   char **strv3;
   int num;
   int_32 *numv;

   headerGetEntry(hdr, RPMTAG_NAME, &type, (void **)&str, &count);
   fprintf(f, "Package: %s\n", str);
   
   headerGetEntry(hdr, RPMTAG_SIZE, &type, (void **)&str, &count);
   fprintf(f, "Version: %s\n", str);
   headerGetEntry(hdr, RPMTAG_ARCHIVESIZE, &type, (void **)&str, &count);
   fprintf(f, "Version: %s\n", str);
    
    
    return;
   headerGetEntry(hdr, RPMTAG_VERSION, &type, (void **)&str, &count);
   fprintf(f, "Version: %s\n", str);

   headerGetEntry(hdr, RPMTAG_RELEASE, &type, (void **)&str, &count);
   fprintf(f, "Release: %s\n", str);

   headerGetEntry(hdr, RPMTAG_GROUP, &type, (void **)&str, &count);
   fprintf(f, "Section: %s\n", str);
   
   headerGetEntry(hdr, RPMTAG_PACKAGER, &type, (void **)&str, &count);
   if (str)
     fprintf(f, "Maintainer: %s\n", str);
   
   headerGetEntry(hdr, RPMTAG_REQUIRENAME, &type, (void **)&strv, &count);
   headerGetEntry(hdr, RPMTAG_REQUIREVERSION, &type2, (void **)&strv2, &count);
   headerGetEntry(hdr, RPMTAG_REQUIREFLAGS, &type3, (void **)&numv, &count);
   if (count > 0) {
      char **dn, **dv;
      int *df;
      int i, j;

      dn = (char**)malloc(sizeof(char*)*count);
      dv = (char**)malloc(sizeof(char*)*count);
      df = (int*)malloc(sizeof(int)*count);
      
      if (!dn || !dv || !df) {
	 puts("could not malloc");
	 exit(1);
      }
      
      for (j = i = 0; i < count; i++) {
	 if ((numv[i] & RPMSENSE_PREREQ) && *strv[i]!='/') {//XXX
	    dn[j] = strv[i];
	    dv[j] = strv2[i];
	    df[j] = numv[i];
	    j++;
	 }
      }
      if (j > 0) {
	 fprintf(f, "Pre-Depends: ");
	 pdepend(f, dn, dv, df, j, type);
      }
      
      for (j = i = 0; i < count; i++) {
	 if (!(numv[i] & RPMSENSE_PREREQ) && *strv[i]!='/') {//XXX
	    dn[j] = strv[i];
	    dv[j] = strv2[i];
	    df[j] = numv[i];
	    j++;
	 }
      }
      if (j > 0) {
	 fprintf(f, "Depends: ");
	 pdepend(f, dn, dv, df, j, type);
      }

      free(dn);
      free(dv);
      free(df);
   }
   
   headerGetEntry(hdr, RPMTAG_CONFLICTNAME, &type, (void **)&strv, &count);
   headerGetEntry(hdr, RPMTAG_CONFLICTVERSION, &type2, (void **)&strv2, &count);
   headerGetEntry(hdr, RPMTAG_CONFLICTFLAGS, &type3, (void **)&numv, &count);
   if (count > 0) {
      fprintf(f, "Conflicts: ");
      pdepend(f, strv, strv2, numv, count, type);
   }
   
   headerGetEntry(hdr, RPMTAG_PROVIDENAME, &type, (void **)&strv, &count);
   if (count > 0) {
      fprintf(f, "Provides: ");
      pstr(f, strv, count, type);
   }

   headerGetEntry(hdr, RPMTAG_OBSOLETENAME, &type, (void **)&strv, &count);
   if (count > 0) {
      fprintf(f, "Replaces: ");//akk??
      pstr(f, strv, count, type);
   }

   
   headerGetEntry(hdr, RPMTAG_ARCH, &type, (void **)&str, &count);
   fprintf(f, "Architecture: %s\n", str);

//   if (headerGetEntry(hdr, RPMTAG_SIZE, &type, (void **)&num, &count)!=0)
//     puts("nao tem archive size");

   
   headerGetEntry(hdr, CRPMTAG_FILENAME, &type, (void **)&str, &count);
   {
      struct stat stbuf;
      char buf[1024];
      
      sprintf(buf, "%s/%s", dir, str);
      if (stat(buf, &stbuf) < 0) {
	 perror(buf);
      }
      num = stbuf.st_size;
   }
   fprintf(f, "Size: %i\n", num / 1000);


   fprintf(f, "Filename: %s\n", str);

   if (headerGetEntry(hdr, RPMSIGTAG_MD5, &type, (void **)&str, &count)==0)
      printf("MD5sum: %s\n", str);

   headerGetEntry(hdr, RPMTAG_SUMMARY, &type, (void **)&str, &count);
   fprintf(f, "Description: %s\n", str);
   
   headerGetEntry(hdr, RPMTAG_DESCRIPTION, &type, (void **)&str, &count);
   dumpDescription(f, str);

   if (headerGetEntry(hdr, RPMTAG_SIZE, &type, (void **)&num, &count))
     fprintf(f, "installed-size: %i ???\n", num/1024);
   else
     fprintf(f, "installed-size: %i (%i)\n", num/1024, type);

   fputc('\n', f);
}


int convert(char *dir, char *ifile, char *ofile)
{
   int fd;
   Header h;
   FILE *fout;
   
   
   fd = open(ifile, O_RDONLY);
   if (fd < 0) {
      puts("cant open hdlist.");
      perror(ifile);
      exit(1);
   }
   
   fout = fopen(ofile, "w+");

   while (1) {
      FD_t fdt = fdDup(fd);
      h = headerRead(fdt, HEADER_MAGIC_YES);
      fdClose(fdt);
      if (!h)
	break;
      showHeader(fout, h, dir);
      
   }
   fclose(fout);

   close(fd);
}



void findFileRequires(Header hdr)
{
   int type, count;
   char *name;
   char **strv;
   int i;

   headerGetEntry(hdr, RPMTAG_NAME, &type, (void **)&name, &count);   

   headerGetEntry(hdr, RPMTAG_REQUIRENAME, &type, (void **)&strv, &count);
   for (i = 0; i < count; i++) {
      char *s;
      
      s = strv[i];
      if (*s == '/') {
	 HashInsert(table, s, s);
      }
   }   
}




void findFileProviders(Header hdr)
{
   int type, count;
   char *name;
   const char **strv;
   int i;

   headerGetEntry(hdr, RPMTAG_NAME, &type, (void **)&name, &count);   

   rpmBuildFileList(hdr, &strv, &count);
   for (i = 0; i < count; i++) {
      const char *s;
      
      s = strv[i];
      if (HashGet(table, s)) {
	 HashInsert(table, (char*)s, name);
      }
   }   
}



void preprocess(char *ifile)
{
   int fd;
   Header h;
      
   fd = open(ifile, O_RDONLY);
   if (fd < 0) {
      puts("cant open hdlist.");
      perror(ifile);
      exit(1);
   }
   
   
   
   while (1) {
      FD_t fdt = fdDup(fd);
      h = headerRead(fdt, HEADER_MAGIC_YES);
      fdClose(fdt);
      if (!h)
	break;
      findFileRequires(h);
   }
   
   close(fd);
   fd = open(ifile, O_RDONLY);

   while (1) {
      FD_t fdt = fdDup(fd);
      h = headerRead(fdt, HEADER_MAGIC_YES);
      fdClose(fdt);
      if (!h)
	break;
      findFileProviders(h);
   }

   close(fd);
}




int main(int argc, char **argv)
{
   if (argc < 2) {
      puts("you need to supply a list of hdlists");
   } else {
      int i;
      char fname[512], fname2[512], dir[512];
       
            
      for (i = 1; i < argc; i++) {
	 table = CreateHashTable(StringHashCallbacks);

	 // look for file dependencies
//	 preprocess(argv[i]);
	 
	 // convert the files
	 sprintf(fname, "%s/hdlist.001", argv[i]);
	 sprintf(fname2, "%s/Packages.001", argv[i]);
	 sprintf(dir, "%s/../RPMS.001", argv[i]);
	 convert(dir, fname, fname2);

	 sprintf(fname, "%s/hdlist.002", argv[i]);
	 sprintf(fname2, "%s/Packages.002", argv[i]);
	 sprintf(dir, "%s/../RPMS.002", argv[i]);
	 convert(dir, fname, fname2);

	 FreeHashTable(table);
      }
   }
}
