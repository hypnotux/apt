// CNC:2003-03-15
// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: repository.cc,v 1.4 2002/07/29 18:13:52 niemeyer Exp $
/* ######################################################################

   Lua interface system.
   
   ##################################################################### */
									/*}}}*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/luaiface.h"
#endif       

#include <config.h>

#ifdef WITH_LUA

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lposix.h"
#include "lrexlib.h"
#include "linit.h"

// For the interactive interpreter.
#define main lua_c_main
#define readline lua_c_readline
#define L lua_c_L
#include "../lua/lua/lua.c"
#undef main
#undef readline
#undef L
}

#include <apt-pkg/depcache.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/error.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/sptr.h>
#include <apt-pkg/version.h>
#include <apt-pkg/pkgsystem.h>

#include <apt-pkg/luaiface.h>

#include <apti18n.h>

#include <limits.h>
#include <unistd.h>
#include <sys/types.h>

#define pushudata(ctype, value) \
   do { \
      ctype *_tmp = (ctype *) lua_newuserdata(L, sizeof(ctype)); \
      *_tmp = (ctype) value; \
      luaL_getmetatable(L, #ctype); \
      lua_setmetatable(L, -2); \
   } while (0)

#define checkudata(ctype, target, n) \
   do { \
      ctype *_tmp = (ctype *) luaL_checkudata(L, n, #ctype); \
      if (_tmp != NULL) \
	 target = *_tmp; \
      else \
	 target = NULL; \
   } while (0)

Lua *_GetLuaObj()
{
   static Lua *Obj = new Lua;
   return Obj;
}

static int luaopen_apt(lua_State *L);

const double Lua::NoGlobalI = INT_MIN;

Lua::Lua()
      : DepCache(0), Cache(0), CacheControl(0), Fix(0), DontFix(0)
{
   _config->CndSet("Dir::Bin::scripts", "/usr/lib/apt/scripts");

   const luaL_reg lualibs[] = {
      {"base", luaopen_base},
      {"table", luaopen_table},
      {"io", luaopen_io},
      {"string", luaopen_string},
      {"math", luaopen_math},
      {"debug", luaopen_debug},
      {"loadlib", luaopen_loadlib},
      {"posix", luaopen_posix},
      {"rex", luaopen_rex},
      {"init", luaopen_init},
      {"apt", luaopen_apt},
      {NULL, NULL}
   };
   L = lua_open();
   const luaL_reg *lib = lualibs;
   for (; lib->name; lib++) {
      lib->func(L);  /* open library */
      lua_settop(L, 0);  /* discard any results */
   }
   luaL_newmetatable(L, "pkgCache::Package*");
   luaL_newmetatable(L, "pkgCache::Version*");
   lua_pop(L, 2);
}

Lua::~Lua()
{
   if (CacheControl)
      CacheControl->Close();
   lua_close(L);
}

bool Lua::HasScripts(const char *ConfListKey)
{
   const Configuration::Item *Top = _config->Tree(ConfListKey);
   if (Top != 0 && Top->Child != 0)
      return true;
   return false;
}

bool Lua::RunScripts(const char *ConfListKey, bool CacheChunks)
{
   lua_pushstring(L, "script_slot");
   lua_pushstring(L, ConfListKey);
   lua_rawset(L, LUA_GLOBALSINDEX);

   CacheData Data;
   if (ChunkCache.find(ConfListKey) == ChunkCache.end()) {
      string File, Dir = _config->FindDir("Dir::Bin::scripts", "");
      Data.Begin = lua_gettop(L)+1;
      const Configuration::Item *Top = _config->Tree(ConfListKey);
      for (Top = (Top == 0?0:Top->Child); Top != 0; Top = Top->Next) {
	 const string &Value = Top->Value;
	 if (Value.empty() == true)
	    continue;
	 if (Value == "interactive") {
	    lua_pushstring(L, "script_filename");
	    lua_pushstring(L, "<interactive>");
	    lua_rawset(L, LUA_GLOBALSINDEX);
	    cout << endl
	         << "APT Interactive " << LUA_VERSION << " Interpreter"
		 << endl << "[" << ConfListKey << "]" << endl;
	    int OldTop = lua_gettop(L);
	    lua_c_L = L;
	    manual_input();
	    lua_settop(L, OldTop);
	    continue;
	 }
	 if (Value[0] == '.' || Value[0] == '/') {
	    if (FileExists(Value) == true)
	       File = Value;
	    else
	       continue;
	 } else {
	    File = Dir+Value;
	    if (FileExists(File) == false)
	       continue;
	 }
	 lua_pushstring(L, "script_filename");
	 lua_pushstring(L, File.c_str());
	 lua_rawset(L, LUA_GLOBALSINDEX);
	 if (luaL_loadfile(L, File.c_str()) != 0) {
	    _error->Warning(_("Error loading script: %s"),
			    lua_tostring(L, -1));
	    lua_remove(L, -1);
	 }
      }
      Data.End = lua_gettop(L)+1;
      if (Data.Begin == Data.End)
	 return false;
      if (CacheChunks == true)
	 ChunkCache[ConfListKey] = Data;
   } else {
      Data = ChunkCache[ConfListKey];
   }
   for (int i = Data.Begin; i != Data.End; i++) {
      lua_pushvalue(L, i);
      if (lua_pcall(L, 0, 0, 0) != 0) {
	 _error->Warning(_("Error running script: %s"),
			 lua_tostring(L, -1));
	 lua_remove(L, -1);
      }
   }
   if (CacheChunks == false) {
      for (int i = Data.Begin; i != Data.End; i++)
	 lua_remove(L, Data.Begin);
   }
   return true;
}

void Lua::SetGlobal(const char *Name)
{
   lua_pushstring(L, Name);
   lua_pushnil(L);
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, const char *Value)
{
   if (Value != NULL) {
      lua_pushstring(L, Name);
      lua_pushstring(L, Value);
      lua_rawset(L, LUA_GLOBALSINDEX);
   }
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, pkgCache::Package *Value)
{
   if (Value != NULL) {
      lua_pushstring(L, Name);
      pushudata(pkgCache::Package*, Value);
      lua_rawset(L, LUA_GLOBALSINDEX);
   }
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, const char **Value, int Total)
{
   lua_pushstring(L, Name);
   lua_newtable(L);
   if (Total == -1)
      Total = INT_MAX;
   for (int i=0; i != Total && Value[i] != NULL; i++) {
      lua_pushstring(L, Value[i]);
      lua_rawseti(L, -2, i+1);
   }
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, vector<const char *> &Value,
		    int Total)
{
   lua_pushstring(L, Name);
   lua_newtable(L);
   if (Total == -1 || Total > Value.size())
      Total = Value.size();
   for (int i=0; i != Total && Value[i] != NULL; i++) {
      lua_pushstring(L, Value[i]);
      lua_rawseti(L, -2, i+1);
   }
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, vector<string> &Value,
		    int Total)
{
   lua_pushstring(L, Name);
   lua_newtable(L);
   if (Total == -1 || Total > Value.size())
      Total = Value.size();
   for (int i=0; i != Total; i++) {
      lua_pushstring(L, Value[i].c_str());
      lua_rawseti(L, -2, i+1);
   }
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, vector<pkgCache::Package*> &Value,
		    int Total)
{
   lua_pushstring(L, Name);
   lua_newtable(L);
   if (Total == -1 || Total > Value.size())
      Total = Value.size();
   for (int i=0; i != Total && Value[i] != NULL; i++) {
      pushudata(pkgCache::Package*, Value[i]);
      lua_rawseti(L, -2, i+1);
   }
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, bool Value)
{
   lua_pushstring(L, Name);
   lua_pushboolean(L, Value);
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, double Value)
{
   lua_pushstring(L, Name);
   lua_pushnumber(L, Value);
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, void *Value)
{
   if (Value != NULL) {
      lua_pushstring(L, Name);
      lua_pushlightuserdata(L, Value);
      lua_rawset(L, LUA_GLOBALSINDEX);
   }
   Globals.push_back(Name);
}

void Lua::SetGlobal(const char *Name, lua_CFunction Value)
{
   lua_pushstring(L, Name);
   lua_pushcfunction(L, Value);
   lua_rawset(L, LUA_GLOBALSINDEX);
   Globals.push_back(Name);
}

void Lua::ResetGlobals()
{
   if (Globals.empty() == false) {
      for (vector<string>::const_iterator I = Globals.begin();
	   I != Globals.end(); I++) {
	 lua_pushstring(L, I->c_str());
	 lua_pushnil(L);
	 lua_rawset(L, LUA_GLOBALSINDEX);
      }
      Globals.clear();
   }
}

const char *Lua::GetGlobalStr(const char *Name)
{
   lua_pushstring(L, Name);
   lua_rawget(L, LUA_GLOBALSINDEX);
   const char *Ret = NULL;
   if (lua_isstring(L, -1))
      Ret = lua_tostring(L, -1);
   lua_remove(L, -1);
   return Ret;
}

void Lua::GetGlobalVStr(const char *Name, vector<string> &VS)
{
   lua_pushstring(L, Name);
   lua_rawget(L, LUA_GLOBALSINDEX);
   int t = lua_gettop(L);
   if (lua_istable(L, t)) {
      lua_pushnil(L);
      while (lua_next(L, t) != 0) {
	 if (lua_isstring(L, -1))
	    VS.push_back(lua_tostring(L, -1));
	 lua_pop(L, 1);
      }
   }
   lua_remove(L, -1);
}

double Lua::GetGlobalNum(const char *Name)
{
   lua_pushstring(L, Name);
   lua_rawget(L, LUA_GLOBALSINDEX);
   double Ret = NoGlobalI;
   if (lua_isnumber(L, -1))
      Ret = lua_tonumber(L, -1);
   lua_remove(L, -1);
   return Ret;
}

void *Lua::GetGlobalPtr(const char *Name)
{
   lua_pushstring(L, Name);
   lua_rawget(L, LUA_GLOBALSINDEX);
   void *Ret = NULL;
   if (lua_isuserdata(L, -1))
      Ret = lua_touserdata(L, -1);
   lua_remove(L, -1);
   return Ret;
}

pkgCache::Package *Lua::GetGlobalPkg(const char *Name)
{
   lua_pushstring(L, Name);
   lua_rawget(L, LUA_GLOBALSINDEX);
   pkgCache::Package *Ret;
   checkudata(pkgCache::Package*, Ret, -1);
   lua_remove(L, -1);
   return Ret;
}

void Lua::SetDepCache(pkgDepCache *DepCache_)
{
   DepCache = DepCache_;
   if (DepCache != NULL)
      Cache = &DepCache->GetCache();
   else
      Cache = NULL;
}

void Lua::SetCacheControl(LuaCacheControl *CacheControl_)
{
   CacheControl = CacheControl_;
}

pkgDepCache *Lua::GetDepCache(lua_State *L)
{
   if (DepCache == NULL && CacheControl)
      SetDepCache(CacheControl->Open());
   if (DepCache == NULL && L != NULL) {
      lua_pushstring(L, "no depcache available at that point");
      lua_error(L);
   }
   return DepCache;
}

pkgCache *Lua::GetCache(lua_State *L)
{
   if (Cache == NULL && CacheControl)
      SetDepCache(CacheControl->Open());
   if (Cache == NULL && L != NULL) {
      lua_pushstring(L, "no cache available at that point");
      lua_error(L);
   }
   return Cache;
}

inline pkgCache::Package *AptAux_ToPackage(lua_State *L, int n)
{
   if (lua_isstring(L, n)) {
      pkgCache *Cache = _lua->GetCache(L);
      if (Cache == NULL)
	 return NULL;
      const char *Name = lua_tostring(L, n);
      return (pkgCache::Package*)Cache->FindPackage(Name);
   } else {
      pkgCache::Package *Pkg;
      checkudata(pkgCache::Package*, Pkg, n);
      if (Pkg == NULL)
	 luaL_argerror(L, n, "invalid package");
      return Pkg;
   }
}

static pkgCache::PkgIterator *AptAux_ToPkgIterator(lua_State *L, int n)
{
   pkgCache::Package *Pkg = AptAux_ToPackage(L, n);
   if (Pkg == NULL)
      return NULL;
   pkgCache *Cache = _lua->GetCache(L);
   if (Cache == NULL)
      return NULL;
   return new pkgCache::PkgIterator(*Cache, Pkg);
}

inline pkgCache::Version *AptAux_ToVersion(lua_State *L, int n)
{
   pkgCache::Version *Ver;
   checkudata(pkgCache::Version*, Ver, n);
   if (Ver == NULL)
      luaL_argerror(L, n, "invalid version");
   return Ver;
}

static pkgCache::VerIterator *AptAux_ToVerIterator(lua_State *L, int n)
{
   pkgCache::Version *Ver = AptAux_ToVersion(L, n);
   if (Ver == NULL)
      return NULL;
   pkgCache *Cache = _lua->GetCache(L);
   if (Cache == NULL)
      return NULL;
   return new pkgCache::VerIterator(*Cache, Ver);
}

inline int AptAux_PushPackage(lua_State *L, pkgCache::Package *Pkg)
{
   if (Pkg != 0) {
      pushudata(pkgCache::Package*, Pkg);
      return 1;
   }
   return 0;
}

inline int AptAux_PushVersion(lua_State *L, pkgCache::Version *Ver)
{
   if (Ver != 0) {
      pushudata(pkgCache::Version*, Ver);
      return 1;
   }
   return 0;
}

static int AptAux_PushVersion(lua_State *L, map_ptrloc Loc)
{
   pkgCache *Cache = _lua->GetCache(L);
   if (Cache == NULL)
      return 0;
   if (Loc != 0) {
      pkgCache::Version *Ver = Cache->VerP+Loc;
      if (Ver != 0) {
	 pushudata(pkgCache::Version*, Ver);
	 return 1;
      }
   }
   return 0;
}

static int AptAux_PushCacheString(lua_State *L, map_ptrloc Pos)
{
   if (Pos == 0)
      return 0;
   pkgCache *Cache = _lua->GetCache(L);
   if (Cache == NULL)
      return 0;
   const char *Str = Cache->StrP+Pos;
   lua_pushstring(L, Str);
   return 1;
}

inline int AptAux_PushBool(lua_State *L, bool Value)
{
   if (Value == true) {
      lua_pushnumber(L, 1);
      return 1;
   }
   return 0;
}

#define MARK_KEEP    0
#define MARK_INSTALL 1
#define MARK_REMOVE  2

static int AptAux_mark(lua_State *L, int Kind)
{
   pkgCache::Package *Pkg = AptAux_ToPackage(L, 1);
   if (Pkg != NULL) {
      pkgDepCache *DepCache = _lua->GetDepCache(L);
      if (DepCache == NULL)
	 return 0;
      pkgProblemResolver *MyFix = NULL;
      pkgProblemResolver *Fix = _lua->GetProblemResolver();
      if (Fix == NULL)
	 Fix = MyFix = new pkgProblemResolver(DepCache);
      pkgCache::PkgIterator PkgI(DepCache->GetCache(), Pkg);
      Fix->Clear(PkgI);
      Fix->Protect(PkgI);
      switch (Kind) {
	 case MARK_KEEP:
	    DepCache->MarkKeep(PkgI);
	    break;
	 case MARK_INSTALL:
	    DepCache->MarkInstall(PkgI);
	    break;
	 case MARK_REMOVE:
	    Fix->Remove(PkgI);
	    DepCache->MarkDelete(PkgI);
	    break;
      }
      if (_lua->GetDontFix() == false && DepCache->BrokenCount() > 0) {
	 if (Kind != MARK_KEEP) {
	    Fix->Resolve(false);
	 } else {
	    Fix->InstallProtect();
	    Fix->Resolve(true);
	 }
      }
      delete MyFix;
   }
   return 0;
}

static int AptLua_confget(lua_State *L)
{
   const char *key = luaL_checkstring(L, 1);
   const char *def = luaL_optstring(L, 2, "");
   string Value;
   if (key != NULL)
      Value = _config->FindAny(key, def);
   lua_pushstring(L, Value.c_str());
   return 1;
}

static int AptLua_confgetlist(lua_State *L)
{
   const char *key = luaL_checkstring(L, 1);
   if (key == NULL)
      return 0;
   const Configuration::Item *Top = _config->Tree(key);
   lua_newtable(L);
   int i = 1;
   for (Top = (Top == 0?0:Top->Child); Top != 0; Top = Top->Next) {
       if (Top->Value.empty() == true)
          continue;
       lua_pushstring(L, Top->Value.c_str());
       lua_rawseti(L, -2, i++);
   }
   return 1;
}

static int AptLua_confset(lua_State *L)
{
   const char *key = luaL_checkstring(L, 1);
   const char *val = luaL_checkstring(L, 2);
   const int cnd = luaL_optint(L, 3, 0);
   if (key != NULL && val != NULL) {
      if (cnd != 0)
	 _config->CndSet(key, val);
      else
	 _config->Set(key, val);
   }
   return 0;
}

static int AptLua_confexists(lua_State *L)
{
   const char *key = luaL_checkstring(L, 1);
   if (key == NULL)
      return 0;
   if (_config->Exists(key) == true)
      lua_pushnumber(L, 1);
   else
      lua_pushnil(L);
   return 1;
}

static int AptLua_confclear(lua_State *L)
{
   const char *key = luaL_checkstring(L, 1);
   if (key != NULL)
      _config->Clear(key);
   return 0;
}


static int AptLua_pkgfind(lua_State *L)
{
   const char *name = luaL_checkstring(L, 1);
   if (name == NULL)
      return 0;
   pkgCache *Cache = _lua->GetCache(L);
   if (Cache == NULL)
      return 0;
   return AptAux_PushPackage(L, Cache->FindPackage(name));
}

static int AptLua_pkglist(lua_State *L)
{
   pkgCache *Cache = _lua->GetCache(L);
   if (Cache == NULL)
      return 0;
   lua_newtable(L);
   int i = 1;
   for (pkgCache::PkgIterator PkgI = Cache->PkgBegin();
        PkgI.end() == false; PkgI++) {
      pushudata(pkgCache::Package*, PkgI);
      lua_rawseti(L, -2, i++);
   }
   return 1;
}

static int AptLua_pkgname(lua_State *L)
{
   pkgCache::Package *Pkg = AptAux_ToPackage(L, 1);
   if (Pkg == NULL)
      return 0;
   return AptAux_PushCacheString(L, Pkg->Name);
}

static int AptLua_pkgsummary(lua_State *L)
{
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   if ((*PkgI)->VersionList == 0) {
      lua_pushstring(L, "");
   } else {
      pkgCache *Cache = _lua->GetCache(L);
      if (Cache == NULL)
	 return 0;
      pkgRecords Recs(*Cache);
      pkgRecords::Parser &Parse = 
			      Recs.Lookup(PkgI->VersionList().FileList());
      lua_pushstring(L, Parse.ShortDesc().c_str());
   }
   return 1;
}

static int AptLua_pkgdescr(lua_State *L)
{
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   if ((*PkgI)->VersionList == 0) {
      lua_pushstring(L, "");
   } else {
      pkgCache *Cache = _lua->GetCache(L);
      if (Cache == NULL)
	 return 0;
      pkgRecords Recs(*Cache);
      pkgRecords::Parser &Parse = 
			      Recs.Lookup(PkgI->VersionList().FileList());
      lua_pushstring(L, Parse.LongDesc().c_str());
   }
   return 1;
}

static int AptLua_pkgisvirtual(lua_State *L)
{
   pkgCache::Package *Pkg = AptAux_ToPackage(L, 1);
   if (Pkg == NULL)
      return 0;
   if (Pkg->VersionList == 0)
      lua_pushnumber(L, 1);
   else
      lua_pushnil(L);
   return 1;
}

static int AptLua_pkgvercur(lua_State *L)
{
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   int Ret = AptAux_PushVersion(L, (*PkgI)->CurrentVer);
   return Ret;
}

static int AptLua_pkgverinst(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   pkgCache::Version *InstVer = (*DepCache)[(*PkgI)].InstallVer;
   pkgCache::Version *CurVer = (*PkgI).CurrentVer();
   if (InstVer == CurVer)
      InstVer = NULL;
   return AptAux_PushVersion(L, InstVer);
}

static int AptLua_pkgvercand(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   pkgCache::Version *CandVer = (*DepCache)[(*PkgI)].CandidateVer;
   pkgCache::Version *CurVer = (*PkgI).CurrentVer();
   if (CandVer == CurVer)
      CandVer = NULL;
   return AptAux_PushVersion(L, CandVer);
}

static int AptLua_pkgverlist(lua_State *L)
{
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   lua_newtable(L);
   int i = 1;
   for (pkgCache::VerIterator Ver = (*PkgI).VersionList();
        Ver.end() == false; Ver++) {
      pushudata(pkgCache::Version*, Ver);
      lua_rawseti(L, -2, i++);
   }
   return 1;
}

static int AptLua_verstr(lua_State *L)
{
   pkgCache::Version *Ver = AptAux_ToVersion(L, 1);
   return AptAux_PushCacheString(L, Ver->VerStr);
}

static int AptLua_verarch(lua_State *L)
{
   pkgCache::Version *Ver = AptAux_ToVersion(L, 1);
   return AptAux_PushCacheString(L, Ver->Arch);
   
}

static int AptLua_verisonline(lua_State *L)
{
   pkgCache::VerIterator *VerI = AptAux_ToVerIterator(L, 1);
   return AptAux_PushBool(L, VerI->Downloadable());
}

static int AptLua_verprovlist(lua_State *L)
{
   pkgCache::VerIterator *VerI = AptAux_ToVerIterator(L, 1);
   if (VerI == NULL)
      return 0;
   pkgCache::PrvIterator PrvI = VerI->ProvidesList();
   lua_newtable(L);
   int i = 1;
   for (; PrvI.end() == false; PrvI++)
   {
      lua_newtable(L);
      lua_pushstring(L, "name");
      lua_pushstring(L, PrvI.Name());
      lua_settable(L, -3);
      lua_pushstring(L, "version");
      if (PrvI.ProvideVersion())
         lua_pushstring(L, PrvI.ProvideVersion());
      else
         lua_pushstring(L, "");
      lua_settable(L, -3);
      lua_rawseti(L, -2, i++);
   }
   return 1;
}

static int AptLua_verstrcmp(lua_State *L)
{
   const char *Ver1, *Ver2;
   const char *Arch1, *Arch2;
   int Top = lua_gettop(L);
   int Ret = -9999;
   bool Error = false;
   if (Top == 2) {
      Ver1 = luaL_checkstring(L, 1);
      Ver2 = luaL_checkstring(L, 2);
      if (Ver1 == NULL || Ver2 == NULL)
	 Error = true;
      else
	 Ret = _system->VS->CmpVersion(Ver1, Ver2);
   } else if (Top == 4) {
      Ver1 = luaL_checkstring(L, 1);
      Arch1 = luaL_checkstring(L, 2);
      Ver2 = luaL_checkstring(L, 3);
      Arch2 = luaL_checkstring(L, 4);
      if (Ver1 == NULL || Arch1 == NULL || Ver2 == NULL || Arch2 == NULL)
	 Error = true;
      else
	 Ret = _system->VS->CmpVersionArch(Ver1, Arch1, Ver2, Arch2);
   } else {
      Error = true;
   }
   if (Error == true) {
      lua_pushstring(L, "verstrcmp requires 2 or 4 string arguments");
      lua_error(L);
      return 0;
   } else {
      lua_pushnumber(L, Ret);
      return 1;
   }
}

static int AptLua_markkeep(lua_State *L)
{
   return AptAux_mark(L, MARK_KEEP);
}

static int AptLua_markinstall(lua_State *L)
{
   return AptAux_mark(L, MARK_INSTALL);
}

static int AptLua_markremove(lua_State *L)
{
   return AptAux_mark(L, MARK_REMOVE);
}

static int AptLua_markdistupgrade(lua_State *L)
{
   if (lua_gettop(L) != 0) {
      lua_pushstring(L, "markdistupgrade has no arguments");
      lua_error(L);
      return 0;
   }
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache != NULL)
      pkgDistUpgrade(*DepCache);
   return 0;
}

static int AptLua_markupgrade(lua_State *L)
{
   if (lua_gettop(L) != 0) {
      lua_pushstring(L, "markdistupgrade has no arguments");
      lua_error(L);
      return 0;
   }
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache != NULL)
      pkgAllUpgrade(*DepCache);
   return 0;
}

static int AptLua_statkeep(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].Keep());
}

static int AptLua_statinstall(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].Install());
}

static int AptLua_statremove(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].Delete());
}

static int AptLua_statnewinstall(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].NewInstall());
}

static int AptLua_statupgrade(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].Upgrade());
}

static int AptLua_statupgradable(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].Upgradable());
}

static int AptLua_statdowngrade(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].Downgrade());
}

static int AptLua_statnowbroken(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].NowBroken());
}

static int AptLua_statinstbroken(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   return AptAux_PushBool(L, (*DepCache)[*PkgI].InstBroken());
}

static int AptLua_statstr(lua_State *L)
{
   pkgDepCache *DepCache = _lua->GetDepCache(L);
   if (DepCache == NULL)
      return 0;
   SPtr<pkgCache::PkgIterator> PkgI = AptAux_ToPkgIterator(L, 1);
   if (PkgI == NULL)
      return 0;
   pkgDepCache::StateCache &S = (*DepCache)[*PkgI];
   if (S.NewInstall()) {
      if (S.InstBroken()) {
	 lua_pushstring(L, "newinstall(broken)");
      } else {
	 lua_pushstring(L, "newinstall");
      }
   } else if (S.Upgrade()) {
      if (S.InstBroken()) {
	 lua_pushstring(L, "upgrade(broken)");
      } else {
	 lua_pushstring(L, "upgrade");
      }
   } else if (S.Downgrade()) {
      if (S.InstBroken()) {
	 lua_pushstring(L, "downgrade(broken)");
      } else {
	 lua_pushstring(L, "downgrade");
      }
   } else if (S.Keep()) {
      if (S.NowBroken()) {
	 lua_pushstring(L, "keep(broken)");
      } else {
	 lua_pushstring(L, "keep");
      }
   } else if (S.Delete()) {
      lua_pushstring(L, "remove");
   } else {
      lua_pushstring(L, "unknown state in statstr(), "
			"report to the maintainer");
      lua_error(L);
      return 0;
   }
   return 1;
}

static int AptLua_apterror(lua_State *L)
{
   const char *str = luaL_checkstring(L, 1);
   if (str != NULL)
      _error->Error("%s", str);
   return 0;
}

static int AptLua_aptwarning(lua_State *L)
{
   const char *str = luaL_checkstring(L, 1);
   if (str != NULL)
      _error->Warning("%s", str);
   return 0;
}

static const luaL_reg aptlib[] = {
   {"confget",		AptLua_confget},
   {"confgetlist",	AptLua_confgetlist},
   {"confset",		AptLua_confset},
   {"confexists",	AptLua_confexists},
   {"confclear",	AptLua_confclear},
   {"pkgfind",		AptLua_pkgfind},
   {"pkglist",		AptLua_pkglist},
   {"pkgname",		AptLua_pkgname},
   {"pkgsummary",	AptLua_pkgsummary},
   {"pkgdescr",		AptLua_pkgdescr},
   {"pkgisvirtual",	AptLua_pkgisvirtual},
   {"pkgvercur",	AptLua_pkgvercur},
   {"pkgverinst",	AptLua_pkgverinst},
   {"pkgvercand",	AptLua_pkgvercand},
   {"pkgverlist",	AptLua_pkgverlist},
   {"verstr",		AptLua_verstr},
   {"verarch",		AptLua_verarch},
   {"verisonline",	AptLua_verisonline},
   {"verprovlist",   	AptLua_verprovlist},
   {"verstrcmp",	AptLua_verstrcmp},
   {"markkeep",		AptLua_markkeep},
   {"markinstall",	AptLua_markinstall},
   {"markremove",	AptLua_markremove},
   {"markdistupgrade",  AptLua_markdistupgrade},
   {"markupgrade",	AptLua_markupgrade},
   {"statkeep",		AptLua_statkeep},
   {"statinstall",	AptLua_statinstall},
   {"statremove",	AptLua_statremove},
   {"statnewinstall",	AptLua_statnewinstall},
   {"statupgrade",	AptLua_statupgrade},
   {"statupgradable",	AptLua_statupgradable},
   {"statdowngrade",	AptLua_statdowngrade},
   {"statnowbroken",	AptLua_statnowbroken},
   {"statinstbroken",	AptLua_statinstbroken},
   {"statstr",		AptLua_statstr},
   {"apterror",		AptLua_apterror},
   {"aptwarning",	AptLua_aptwarning},
   {NULL, NULL}
};

static int luaopen_apt(lua_State *L)
{
   lua_pushvalue(L, LUA_GLOBALSINDEX);
   luaL_openlib(L, NULL, aptlib, 0);
}

pkgDepCache *LuaCacheControl::Open()
{
   if (geteuid() == 0)
      return Open(true);
   else
      return Open(false);
}

#endif // WITH_LUA

// vim:sts=3:sw=3
