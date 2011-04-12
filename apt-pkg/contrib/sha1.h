// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: sha1.h,v 1.1 2002/07/23 17:54:51 niemeyer Exp $
/* ######################################################################

   Legacy wrapper around raptHash class.

   ##################################################################### */
									/*}}}*/
#ifndef APTPKG_SHA1_H
#define APTPKG_SHA1_H

#include <string>
#include <apt-pkg/rhash.h>

using std::string;

typedef string SHA1SumValue;

class SHA1Summation : public raptHash
{
   public:
   SHA1Summation() : raptHash("SHA1-Hash") {};
};

#endif
