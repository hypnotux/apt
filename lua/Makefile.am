
if WITH_LUA
noinst_LTLIBRARIES = liblua.la
noinst_PROGRAMS = lua/lua luac/luac
endif

EXTRA_DIST = COPYRIGHT HISTORY

LDADD = liblua.la -lm
INCLUDES = -I$(srcdir)/include

lua_lua_SOURCES = lua/lua.c
luac_luac_SOURCES = luac/luac.c luac/print.c lopcodes.c
luac_luac_CFLAGS = -DLUA_OPNAMES

liblua_la_SOURCES = \
		include/lauxlib.h \
		include/lua.h \
		include/lualib.h \
		lib/lauxlib.c \
		lib/lbaselib.c \
		lib/ldblib.c \
		lib/liolib.c \
		lib/lmathlib.c \
		lib/loadlib.c \
		lib/lstrlib.c \
		lib/ltablib.c \
		lapi.c \
		lapi.h \
		lcode.c \
		lcode.h \
		ldebug.c \
		ldebug.h \
		ldo.c \
		ldo.h \
		ldump.c \
		lfunc.c \
		lfunc.h \
		lgc.c \
		lgc.h \
		llex.c \
		llex.h \
		llimits.h \
		lmem.c \
		lmem.h \
		lobject.c \
		lobject.h \
		lopcodes.c \
		lopcodes.h \
		lparser.c \
		lparser.h \
		lstate.c \
		lstate.h \
		lstring.c \
		lstring.h \
		ltable.c \
		ltable.h \
		ltests.c \
		ltm.c \
		ltm.h \
		lundump.c \
		lundump.h \
		lvm.c \
		lvm.h \
		lzio.c \
		lzio.h
