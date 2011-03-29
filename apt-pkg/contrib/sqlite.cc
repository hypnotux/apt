#include <config.h>

#ifdef WITH_SQLITE3

#include "sqlite.h"
#include <apt-pkg/error.h>

#include <cstdlib>

// debug stuff..
#include <iostream>
using namespace std;

SqliteDB::SqliteDB(string DBPath): DB(NULL), DBPath(DBPath)
{
   int rc = sqlite3_open(DBPath.c_str(), &DB);
   if (rc != SQLITE_OK) {
      _error->Error("opening %s db failed", DBPath.c_str());
   }
}

SqliteDB::~SqliteDB()
{
   if (DB) {
      sqlite3_close(DB);
   }
}

SqliteQuery *SqliteDB::Query()
{
   return new SqliteQuery(DB);
}

bool SqliteDB::Exclusive(bool mode)
{
   string cmd = "PRAGMA locking_mode = ";
   cmd += mode ? "EXCLUSIVE" : "NORMAL";
   return (sqlite3_exec(DB, cmd.c_str(), NULL, NULL, NULL) == SQLITE_OK);
}

SqliteQuery::SqliteQuery(sqlite3 *DB) : 
   DB(DB), stmt(NULL), cur(0)
{
}

SqliteQuery::~SqliteQuery()
{
   if (stmt) {
      sqlite3_finalize(stmt);
   }
}

bool SqliteQuery::Exec(const string & SQL)
{
   int rc;

   if (stmt) {
      sqlite3_finalize(stmt);
      stmt = NULL;
      ColNames.clear();
   }

   rc = sqlite3_prepare_v2(DB, SQL.c_str(), SQL.size(), &stmt, NULL);
   return (rc == SQLITE_OK);
}

bool SqliteQuery::Step()
{
   int rc = sqlite3_step(stmt);
   if (rc == SQLITE_ROW) {
      /* Populate column names on first call */
      if (ColNames.empty()) {
	 int ncols = sqlite3_column_count(stmt);
	 for (int i = 0; i < ncols; i++) {
	    ColNames[sqlite3_column_name(stmt, i)] = i; 
	 }
      } else {
	 cur++;
      }
   }
   return (rc == SQLITE_ROW);
}

bool SqliteQuery::Rewind()
{
   int rc = sqlite3_reset(stmt);
   cur = 0;
   return (rc == SQLITE_OK);
}

bool SqliteQuery::Jump(unsigned long Pos)
{
   if (Pos > cur)
      Rewind();
   while (cur < Pos) {
      if (Step() == false)
	 break;
   }
   return (cur == Pos);
}

bool SqliteQuery::Get(const string & ColName, string & Val)
{
   const char *item = (const char *) sqlite3_column_text(stmt, ColNames[ColName]);
   if (item != NULL)
      Val = item;
   return item != NULL;
}

bool SqliteQuery::Get(const string & ColName, unsigned long & Val)
{
   const char *item = (const char *) sqlite3_column_text(stmt, + ColNames[ColName]);
   if (item != NULL)
      Val = atol((const char *) item);
   return item != NULL;
}

string SqliteQuery::GetCol(const string & ColName)
{
   string val = "";
   Get(ColName, val);
   return val;
} 

unsigned long SqliteQuery::GetColI(const string & ColName)
{
   unsigned long val = 0;
   Get(ColName, val);
   return val;
} 

#endif /* WITH_SQLITE3 */


// vim:sts=3:sw=3
