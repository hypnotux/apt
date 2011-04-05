// Include Files
#include <apt-pkg/rhash.h>
#include <apt-pkg/strutl.h>

#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <config.h>
#include <system.h>

raptHash::raptHash(pgpHashAlgo HashAlgo)
{
   HashCtx = rpmDigestInit(HashAlgo, RPMDIGEST_NONE);
}

// raptHash::Result - Return checksum value                        /*{{{*/
// ---------------------------------------------------------------------
/* Add() may not be called after this */
string raptHash::Result()
{
   void *data = NULL;
   size_t dlen = 0;
   string Value = "";
   
   rpmDigestFinal(HashCtx, &data, &dlen, 1);
   if (data) {
      Value = string((const char*)data);
   }
   HashCtx = NULL;

   return Value;
}
									/*}}}*/
// raptHash::Add - Adds content of buffer into the checksum        /*{{{*/
// ---------------------------------------------------------------------
/* May not be called after Result() is called */
bool raptHash::Add(const unsigned char *data,unsigned long len)
{
   int rc;
   if (HashCtx == NULL)
      return false;

   rc = rpmDigestUpdate(HashCtx, data, len);
   
   return (rc == 0);
}
									/*}}}*/
// raptHash::AddFD - Add content of file into the checksum         /*{{{*/
// ---------------------------------------------------------------------
/* */
bool raptHash::AddFD(int Fd,unsigned long Size)
{
   unsigned char Buf[64 * 64];
   int Res = 0;
   while (Size != 0)
   {
      Res = read(Fd,Buf,std::min(Size,(unsigned long)sizeof(Buf)));
      if (Res < 0 || (unsigned) Res != std::min(Size,(unsigned long)sizeof(Buf)))
	 return false;
      Size -= Res;
      Add(Buf,Res);
   }
   return true;
}
									/*}}}*/
