#include <fcntl.h>

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <rpm/rpmlib.h>





int convert(char *ofile)
{
   rpmdb db;
   unsigned int offset;
   Header hdr;
   FILE *f;
   char *rootdir = "/";

   f = fopen(ofile, "w+");
   if (!f) {
      perror(ofile);
      return 0;
   }
   
   rpmdbInit(rootdir, 0644);
   
   if (rpmdbOpen("/", &db, O_RDONLY, 0644)) {
      fclose(f);
      puts("could not open rpmdb");
      return 0;
   }
   
   for (offset = rpmdbFirstRecNum(db);
	offset > 0;
	offset = rpmdbNextRecNum(db, offset)) {
      hdr = rpmdbGetRecord(db, offset);
      
      
   }
   
   rpmdbClose(db);
   fclose(f);
   
   return 1;
}



int main()
{
   convert("status");
}
