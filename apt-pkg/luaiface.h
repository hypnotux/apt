
#ifndef LUAIFACE_H
#define LUAIFACE_H

#ifdef __GNUG__
#pragma interface "apt-pkg/luaiface.h"
#endif 

#ifdef WITH_LUA

#include <map>
#include <vector>

using namespace std;

class pkgCache;
class pkgDepCache;
class pkgProblemResolver;
class lua_State;
typedef int (*lua_CFunction)(struct lua_State*);

class LuaCacheControl {
   public:

   virtual pkgDepCache *Open();
   virtual pkgDepCache *Open(bool Write) = 0;
   virtual void Close() = 0;

   virtual ~LuaCacheControl() {};
};

class Lua {
   protected:

   struct CacheData {
      int Begin;
      int End;
   };

   lua_State *L;
   map<string,CacheData> ChunkCache;

   vector<string> Globals;

   pkgDepCache *DepCache;
   pkgCache *Cache;

   LuaCacheControl *CacheControl;

   pkgProblemResolver *Fix;
   bool DontFix;

   public:

   bool RunScripts(const char *ConfListKey, bool CacheChunks);
   bool HasScripts(const char *ConfListKey);

   void SetGlobal(const char *Name);
   void SetGlobal(const char *Name, const char *Value);
   void SetGlobal(const char *Name, double Value);
   void SetGlobal(const char *Name, void *Value);
   void SetGlobal(const char *Name, string Value)
	 { SetGlobal(Name, Value.c_str()); };
   void SetGlobal(const char *Name, int Value)
	 { SetGlobal(Name, (double)Value); };
   void SetGlobal(const char *Name, lua_CFunction Value);
   void SetGlobal(const char *Name, const char **Value, int Total=-1);
   void SetGlobal(const char *Name, pkgCache::Package *Value);
   void SetGlobal(const char *Name, vector<const char *> &Value,
		  int Total=-1);
   void SetGlobal(const char *Name, vector<string> &Value,
		  int Total=-1);
   void SetGlobal(const char *Name, vector<pkgCache::Package*> &Value,
		  int Total=-1);
   void ResetGlobals();

   const char *GetGlobalStr(const char *Name);
   void GetGlobalVStr(const char *Name, vector<string> &VS);
   double GetGlobalNum(const char *Name);
   void *GetGlobalPtr(const char *Name);
   pkgCache::Package *GetGlobalPkg(const char *Name);

   static const double NoGlobalI;

   void SetDepCache(pkgDepCache *DepCache_);
   void SetCache(pkgCache *Cache_) { Cache = Cache_; };
   void SetCacheControl(LuaCacheControl *CacheControl_);
   void SetProblemResolver(pkgProblemResolver *Fix_) { Fix = Fix_; };
   void SetDontFix() { DontFix = true; };
   void ResetCaches()
      { DepCache = NULL; Cache = NULL; Fix = NULL; DontFix = false; };

   // For API functions
   pkgDepCache *GetDepCache(lua_State *L=NULL);
   pkgCache *GetCache(lua_State *L=NULL);
   LuaCacheControl *GetCacheControl() { return CacheControl; };
   pkgProblemResolver *GetProblemResolver() { return Fix; };
   bool GetDontFix() { return DontFix; };

   Lua();
   ~Lua();
};

// The same system used with _error
Lua *_GetLuaObj();
#define _lua _GetLuaObj()

#endif // WITH_LUA

#endif

// vim:sts=3:sw=3
