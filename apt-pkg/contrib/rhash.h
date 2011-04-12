// Description								/*{{{*/
/* ######################################################################

   This is a C++ interface to rpm's hash (aka digest) functions.

   ##################################################################### */
									/*}}}*/
#ifndef APTPKG_RHASH_H
#define APTPKG_RHASH_H

#include <string>
#include <rpm/rpmpgp.h>

using std::string;

class raptHash
{
   DIGEST_CTX HashCtx;
   string Value;
   
   public:

   bool Add(const unsigned char *inbuf,unsigned long inlen);
   inline bool Add(const char *Data) {return Add((unsigned char *)Data,strlen(Data));}
   bool AddFD(int Fd,unsigned long Size);
   inline bool Add(const unsigned char *Beg,const unsigned char *End) 
                  {return Add(Beg,End-Beg);}
   string Result();
   
   raptHash(const string & HashName);
};

#endif
