// -*- mode: c++; mode: fold -*-
// Description								/*{{{*/
// $Id: md5.h,v 1.1 2002/07/23 17:54:51 niemeyer Exp $
/* ######################################################################
   
   Legacy wrapper around raptHash class.
   
   ##################################################################### */
									/*}}}*/
#ifndef APTPKG_MD5_H
#define APTPKG_MD5_H

#include <string>
#include <cstring>
#include <apt-pkg/rhash.h>

using std::string;

typedef string MD5SumValue;

class MD5Summation : public raptHash
{
   public:
   MD5Summation() : raptHash("MD5-Hash") {};
};


#endif
