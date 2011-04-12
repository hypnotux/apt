// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: hashes.h,v 1.1 2002/07/23 17:54:51 niemeyer Exp $
/* ######################################################################

   Hashes - Simple wrapper around the hash functions
   
   This is just used to make building the methods simpler, this is the
   only interface required..
   
   ##################################################################### */
									/*}}}*/
#ifndef APTPKG_HASHES_H
#define APTPKG_HASHES_H

#include <apt-pkg/rhash.h>

class Hashes
{
   public:

   raptHash MD5;
   raptHash SHA1;
   
   inline bool Add(const unsigned char *Data,unsigned long Size)
   {
      return MD5.Add(Data,Size) && SHA1.Add(Data,Size);
   }
   inline bool Add(const char *Data) {return Add((unsigned char *)Data,strlen(Data));}
   bool AddFD(int Fd,unsigned long Size);
   inline bool Add(const unsigned char *Beg,const unsigned char *End) 
                  {return Add(Beg,End-Beg);}

   Hashes() : MD5("MD5-Hash"), SHA1("SHA1-Hash") {};
};

#endif
