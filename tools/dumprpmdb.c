#include <stdio.h>
#include <fcntl.h>
#include <rpm/rpmlib.h>

int main()
{
   rpmdb db;
   Header header;
   unsigned ofs;

   rpmReadConfigFiles(NULL, NULL);
   
   if (rpmdbOpen(NULL, &db, O_RDONLY, 0644) != 0) {
      puts("couldnt open rpm DB");
      return 0;
   }
   
   
   ofs = rpmdbFirstRecNum(db);
   
   while (ofs!=0) {
      int count;
      int type;
      int num;
      char *str;
  
      header = rpmdbGetRecord(db, ofs);
      if (!header)
	break;

      headerGetEntry(header, RPMTAG_NAME, &type, (void **)&str, &count);
      if (headerGetEntry(header, RPMTAG_SIZE, &type, (void **)&num, &count))
	printf("%s ???\n", str);
      else
	printf("%s %i\n", str, num);
      headerFree(header);

      ofs = rpmdbNextRecNum(db, ofs);
   }
}
  
