/* sqlite-module.c
 * S-Lang bindings for the sqlite3 library
 * This was tested with sqlite3 version 3.5.9
 *
 * Copyright (c) 2006, 2007, 2010, 2017 Paul Boekholt.
 * Released under the terms of the GNU GPL (version 2 or later).
 */
#include "config.h"
#include <sqlite3.h>
#include <slang.h>
#include <string.h>

SLANG_MODULE(sqlite);

#define MODULE_MAJOR_VERSION	0
#define MODULE_MINOR_VERSION	5
#define MODULE_PATCH_LEVEL	1
static char *Module_Version_String = "0.5.1";
#define MODULE_VERSION_NUMBER	\
   (MODULE_MAJOR_VERSION*10000+MODULE_MINOR_VERSION*100+MODULE_PATCH_LEVEL)

/*{{{ sqlite type */

static int DB_Type_Id = 0;

typedef struct
{
   sqlite3 * db;
} db_type;

#define DUMMY_SQLITE_TYPE 255
/*}}}*/
/*{{{ Statement type */

static int Statement_Type_Id = 0;
typedef struct
{
   sqlite3_stmt * ppStmt;
   /*
    * The state of the prepared statement determines which functions are permissible.
    * state		function	end-state
    * -----------	-------------	------------------------
    * SQLITE_OK 	step 		SQLITE_ROW or SQLITE_DONE
    * 			reset		SQLITE_OK
    * 			bind_param(s)	SQLITE_OK
    * SQLITE_ROW 	step		SQLITE_ROW or SQLITE_DONE
    * 			fetch		SQLITE_ROW
    * 			reset		SQLITE_OK
    * SQLITE_DONE	reset		SQLITE_OK
    */
   int state;
} Statement_Type;

#define DUMMY_STATEMENT_TYPE 254

/*}}}*/
/*{{{ exceptions */
static int  Sqlite_Error = 0;
static int  Sqlite_Internal_Error = 0;
static int  Sqlite_Perm_Error = 0;
static int  Sqlite_Abort_Error = 0;
static int  Sqlite_Busy_Error = 0;
static int  Sqlite_Locked_Error = 0;
static int  Sqlite_Nomem_Error = 0;
static int  Sqlite_Readonly_Error = 0;
static int  Sqlite_Interrupt_Error = 0;
static int  Sqlite_Ioerr_Error = 0;
static int  Sqlite_Corrupt_Error = 0;
static int  Sqlite_Notfound_Error = 0;
static int  Sqlite_Full_Error = 0;
static int  Sqlite_Cantopen_Error = 0;
static int  Sqlite_Protocol_Error = 0;
static int  Sqlite_Empty_Error = 0;
static int  Sqlite_Schema_Error = 0;
static int  Sqlite_Toobig_Error = 0;
static int  Sqlite_Constraint_Error = 0;
static int  Sqlite_Mismatch_Error = 0;
static int  Sqlite_Misuse_Error = 0;
static int  Sqlite_Nolfs_Error = 0;
static int  Sqlite_Auth_Error = 0;
static int  Sqlite_Format_Error = 0;
static int  Sqlite_Range_Error = 0;
static int  Sqlite_Notadb_Error = 0;

typedef struct
{
   int error_code;
   int *errcode_ptr;
   char *name;
   char *description;
}
Sqlite_Exception_Table_Type;

static const Sqlite_Exception_Table_Type Sqlite_Exception_Table [] =
{
     { SQLITE_ERROR, &Sqlite_Error, "SqliteError", "Sqlite Error"},
     { SQLITE_INTERNAL, &Sqlite_Internal_Error, "SqliteInternalError", "NOT USED. Internal logic error in SQLite"},
     { SQLITE_PERM, &Sqlite_Perm_Error, "SqlitePermError", "Access permission denied"},
     { SQLITE_ABORT, &Sqlite_Abort_Error, "SqliteAbortError", "Callback routine requested an abort"},
     { SQLITE_BUSY, &Sqlite_Busy_Error, "SqliteBusyError", "The database file is locked"},
     { SQLITE_LOCKED, &Sqlite_Locked_Error, "SqliteLockedError", "A table in the database is locked"},
     { SQLITE_NOMEM, &Sqlite_Nomem_Error, "SqliteNomemError", "A malloc() failed"},
     { SQLITE_READONLY, &Sqlite_Readonly_Error, "SqliteReadonlyError", "Attempt to write a readonly database"},
     { SQLITE_INTERRUPT, &Sqlite_Interrupt_Error, "SqliteInterruptError", "Operation terminated by sqlite3_interrupt()*/"},
     { SQLITE_IOERR, &Sqlite_Ioerr_Error, "SqliteIoerrError", "Some kind of disk I/O error occurred"},
     { SQLITE_CORRUPT, &Sqlite_Corrupt_Error, "SqliteCorruptError", "The database disk image is malformed"},
     { SQLITE_NOTFOUND, &Sqlite_Notfound_Error, "SqliteNotfoundError", "NOT USED. Table or record not found"},
     { SQLITE_FULL, &Sqlite_Full_Error, "SqliteFullError", "Insertion failed because database is full"},
     { SQLITE_CANTOPEN, &Sqlite_Cantopen_Error, "SqliteCantopenError", "Unable to open the database file"},
     { SQLITE_PROTOCOL, &Sqlite_Protocol_Error, "SqliteProtocolError", "Database lock protocol error"},
     { SQLITE_EMPTY, &Sqlite_Empty_Error, "SqliteEmptyError", "Database is empty"},
     { SQLITE_SCHEMA, &Sqlite_Schema_Error, "SqliteSchemaError", "The database schema changed"},
     { SQLITE_TOOBIG, &Sqlite_Toobig_Error, "SqliteToobigError", "NOT USED. Too much data for one row"},
     { SQLITE_CONSTRAINT, &Sqlite_Constraint_Error, "SqliteConstraintError", "Abort due to contraint violation"},
     { SQLITE_MISMATCH, &Sqlite_Mismatch_Error, "SqliteMismatchError", "Data type mismatch"},
     { SQLITE_MISUSE, &Sqlite_Misuse_Error, "SqliteMisuseError", "Library used incorrectly"},
     { SQLITE_NOLFS, &Sqlite_Nolfs_Error, "SqliteNolfsError", "Uses OS features not supported on host"},
     { SQLITE_AUTH, &Sqlite_Auth_Error, "SqliteAuthError", "Authorization denied"},
     { SQLITE_FORMAT, &Sqlite_Format_Error, "SqliteFormatError", "Auxiliary database format error"},
     { SQLITE_RANGE, &Sqlite_Range_Error, "SqliteRangeError", "2nd parameter to sqlite3_bind out of range"},
     { SQLITE_NOTADB, &Sqlite_Notadb_Error, "SqliteNotadbError", "File opened that is not a database file"},
     { SQLITE_OK, 0, 0, 0 }
};

static int check_error (sqlite3 *db, int error_code)
{
   const Sqlite_Exception_Table_Type *b;
   int error;
   if (error_code == SQLITE_OK || error_code == SQLITE_DONE || error_code == SQLITE_ROW) return 0;
   b = Sqlite_Exception_Table;

   while (b->errcode_ptr != NULL)
     {
	if (b->error_code == error_code)
	  break;
	b++;
     }
   if (b->errcode_ptr == NULL) error = Sqlite_Error;
   else error = *(b->errcode_ptr);
   SLang_verror (error, "%s", sqlite3_errmsg(db));
   return 1;
}

/*}}}*/
/*{{{ helper functions */
/*{{{ sqlite type */

static void free_db_type (db_type *pt)
{
   if (pt->db != NULL)
     sqlite3_close(pt->db);
   SLfree ((char *) pt);
}

static void destroy_sqlite (SLtype type, VOID_STAR f)
{
   (void) type;
   free_db_type ((db_type *) f);
}

static SLang_MMT_Type *allocate_db_type (sqlite3 *db)
{
   db_type *pt;
   SLang_MMT_Type *mmt;

   pt = (db_type *) SLmalloc (sizeof (db_type));
   if (pt == NULL)
     return NULL;
   memset ((char *) pt, 0, sizeof (db_type));

   pt->db = db;

   if (NULL == (mmt = SLang_create_mmt (DB_Type_Id, (VOID_STAR) pt)))
     {
	free_db_type (pt);
	return NULL;
     }
   return mmt;
}

/*}}}*/
/*{{{ statement type */

static void free_statement_type (Statement_Type *pt)
{
   if (pt->ppStmt != NULL)
     sqlite3_finalize(pt->ppStmt);
   SLfree ((char *) pt);
}

static void destroy_statement (SLtype type, VOID_STAR f)
{
   (void) type;
   free_statement_type ((Statement_Type *) f);
}

static SLang_MMT_Type *allocate_statement_type (sqlite3_stmt *ppStmt)
{
   Statement_Type *pt;
   SLang_MMT_Type *mmt;

   pt = (Statement_Type *) SLmalloc (sizeof (Statement_Type));
   if (pt == NULL)
     return NULL;
   memset ((char *) pt, 0, sizeof (Statement_Type));

   pt->ppStmt = ppStmt;
   pt->state  = SQLITE_OK;

   if (NULL == (mmt = SLang_create_mmt (Statement_Type_Id, (VOID_STAR) pt)))
     {
	free_statement_type (pt);
	return NULL;
     }
   return mmt;
}

static Statement_Type *pop_statement (SLang_MMT_Type **mmtp)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;

   if ((NULL == (mmt = SLang_pop_mmt (Statement_Type_Id)))
       || (NULL == (p = (Statement_Type *)SLang_object_from_mmt (mmt))))
     {
	SLang_free_mmt (mmt);	       /* NULL ok */
	*mmtp = NULL;
	return NULL;
     }

   *mmtp = mmt;
   return p;
}

/*}}}*/

static int do_sqlite_fetch(sqlite3_stmt *ppStmt)
{
   int i, imax;

   imax = sqlite3_data_count (ppStmt);
   for (i = 0; i < imax; i++)
     {
	SLang_BString_Type *bstr;
	int status;

	switch (sqlite3_column_type(ppStmt, i))
	  {
	   case SQLITE_INTEGER:
	     /* FIXME: SQLite integers are signed 64-bit values */
	     status = SLang_push_integer(sqlite3_column_int(ppStmt, i));
	     break;
	   case SQLITE_FLOAT:
	     status = SLang_push_double(sqlite3_column_double(ppStmt, i));
	     break;
	   case SQLITE_TEXT:
	     status = SLang_push_string((char *)sqlite3_column_text(ppStmt, i));
	     break;
	   case SQLITE_BLOB:
	     bstr = SLbstring_create((unsigned char *)sqlite3_column_blob(ppStmt, i),
				     sqlite3_column_bytes(ppStmt, i));
	     if (bstr == NULL)
	       status = -1;
	     else if (bstr != NULL)
	       status = SLang_push_bstring(bstr);
	     SLbstring_free(bstr);
	     break;
	   case SQLITE_NULL:
	     status = SLang_push_null();
	  }

	if (status == -1)
	  return -1;
     }
   return 0;
}

static int do_sqlite_step(sqlite3 *db, sqlite3_stmt *ppStmt)
{
   int res;
   res = sqlite3_step(ppStmt);
   if (res == SQLITE_ROW)
     {
	if (-1 == do_sqlite_fetch(ppStmt))
	  return -1;

	return 1;
     }
   if (res != SQLITE_DONE)
     check_error(db, res);
   return -1;
}

static void slstring_destructor (void *p)
{
   SLang_free_slstring ((SLstr_Type *)p);
}

/*
 * Bind parameters with positions i .. num.
 * Usually, either i = 1 or i = num
 */
static int do_sqlite_bind(sqlite3 *db, sqlite3_stmt *ppStmt, int num, int i)
{
   char *svalue;
   SLang_BString_Type *bvalue;
   unsigned int bstrlen;
   unsigned char *bptr;

#define MAP(slangtype, ctype, slangpop, sqlitebind) \
 case slangtype:\
   {\
      ctype value;\
      slangpop(&value);\
      if(check_error(db, sqlitebind(ppStmt, i, value)))\
	return -1;\
      break;\
   }

   for (; i <= num; i++)
     {
	switch (SLang_peek_at_stack())
	  {
	     MAP(SLANG_INT_TYPE, int, SLang_pop_int, sqlite3_bind_int);
	     MAP(SLANG_FLOAT_TYPE, float, SLang_pop_float, sqlite3_bind_double);
	     MAP(SLANG_DOUBLE_TYPE, double, SLang_pop_double, sqlite3_bind_double);
#ifdef HAVE_LONG_LONG
	   case SLANG_UINT_TYPE:
	       {
		  unsigned int value;
		  SLang_pop_uint(&value);
		  if (check_error(db, sqlite3_bind_int64(ppStmt, i, (long long int) value)))
		    return -1;
		  break;
	       }
	     MAP(SLANG_LLONG_TYPE, long long, SLang_pop_long_long, sqlite3_bind_int64);
#endif
	   case SLANG_STRING_TYPE:
	     SLang_pop_slstring(&svalue);
	     if (check_error(db, sqlite3_bind_text(ppStmt, i, svalue,
                                                  strlen(svalue), slstring_destructor)))
	       return -1;
	     break;
	   case SLANG_BSTRING_TYPE:
	     SLang_pop_bstring(&bvalue);
	     bptr = SLbstring_get_pointer(bvalue, &bstrlen);
	     if (check_error(db, sqlite3_bind_blob(ppStmt, i, bptr,
						   bstrlen, SQLITE_TRANSIENT)))
	       {
		  SLbstring_free(bvalue);
		  return -1;
	       }
	     SLbstring_free(bvalue);
	     break;
	   default:
	     SLdo_pop_n(num + 1 - i);
	     SLang_verror(SL_Usage_Error, "attempt to bind unsupported type");
	     return -1;
	  }
     }
#undef MAP
   return 0;
}

/*}}}*/
/*{{{ exported functions */

static void slsqlite_open(char *name)
{
   SLang_MMT_Type *mmt;
   sqlite3 *db;

   if (check_error(db, sqlite3_open(name, &db))
       || (NULL == (mmt = allocate_db_type (db))))
     {
	(void) SLang_push_null();
	sqlite3_close(db);
	return;
     }

   if (-1 == SLang_push_mmt (mmt))
     {
	SLang_free_mmt (mmt);
	(void) SLang_push_null();
	return;
     }
}

static void slsqlite_prepare (const char *sql)
{
   db_type *p;
   SLang_MMT_Type *mmt, *mmt2;
   sqlite3_stmt *ppStmt;
   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     {
	SLang_free_mmt (mmt);
	(void) SLang_push_null();
	return;
     }
   p = (db_type *)SLang_object_from_mmt (mmt);

   if (check_error(p->db, sqlite3_prepare_v2(p->db, sql, -1, &ppStmt, NULL)))
     {
	goto free_return;
     }
   if (NULL == (mmt2 = allocate_statement_type (ppStmt)))
     {
	(void) SLang_push_null();
	goto free_return;
     }
   if (-1 == SLang_push_mmt (mmt2))
     {
	SLang_free_mmt (mmt2);
	goto free_return;
     }

free_return:
   SLang_free_mmt (mmt);
}

static void slsqlite_get_table (const char *sql)
{
   db_type *p;
   SLang_MMT_Type *mmt;
   SLindex_Type dims[2];
   int nrow, ncolumn;
   char **resultp;
   SLang_Array_Type *at;
   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     {
	SLang_free_mmt (mmt);
	(void) SLang_push_null();
	return;
     }
   p = (db_type *)SLang_object_from_mmt (mmt);

   if (check_error(p->db, sqlite3_get_table(p->db, sql, &resultp, &nrow, &ncolumn, NULL)))
     {
	SLang_free_mmt(mmt);
	return;
     }

   dims[0] = nrow + 1;
   dims[1] = ncolumn;

   at = SLang_create_array(SLANG_STRING_TYPE, 0, NULL, dims, 2);

   if (at != NULL)
     {
       char **pp = resultp;
	SLindex_Type j, k, d[2];
	for (j=0; j<dims[0]; j++)
	  {
	     d[0]=j;
	     for (k=0; k<dims[1]; k++)
	       {
		  d[1]=k;
                 SLang_set_array_element (at, d, pp++);
	       }
	  }

	(void) SLang_push_array (at, 1);
     }
   else
     {
	SLang_free_array (at);
	SLang_push_null();
     }
   sqlite3_free_table(resultp);
   SLang_free_mmt (mmt);
}

static void slsqlite_get_row(void)
{
   db_type *p;
   SLang_MMT_Type *mmt;
   sqlite3_stmt *ppStmt;
   int nargs;
   char *sql;

   nargs = SLang_Num_Function_Args;

   if (nargs < 2)
     {
       (void) SLdo_pop_n (nargs);
	SLang_verror(SL_Usage_Error, "usage: sqlite_get_row(Sqlite db, String sql, ...)");
	return;
     }

   SLreverse_stack(nargs);
   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     {
	SLang_free_mmt (mmt);
	return;
     }
   p = (db_type *)SLang_object_from_mmt (mmt);

   if (-1 == SLang_pop_slstring(&sql))
     {
	SLang_verror(SL_Usage_Error, "usage: sqlite_get_row(Sqlite db, String sql, ...)");
	SLang_free_mmt(mmt);
	return;
     }

   if (check_error(p->db, sqlite3_prepare_v2(p->db, sql, -1, &ppStmt, NULL)))
     {
	goto free_return;
     }

   if (do_sqlite_bind(p->db, ppStmt, nargs - 2, 1))
     {
	sqlite3_finalize(ppStmt);
	goto free_return;
     }

   if (-1 == do_sqlite_step(p->db, ppStmt))
     SLang_verror (Sqlite_Error, "Query returned no result");

   check_error(p->db, sqlite3_finalize (ppStmt));

free_return:
   SLang_free_slstring(sql);
   SLang_free_mmt(mmt);

}

/*{{{ sqlite_get_array */

#define GET_ARRAY(type, ctype, sqltype, slangtype)\
static void sqlite_get_##type##_array(sqlite3_stmt *ppStmt)\
{\
   unsigned int num_items = 0, max_num_items = 1024;\
   SLindex_Type dims[2];\
   ctype *list;\
   SLang_Array_Type *at;\
   list = (ctype *) SLmalloc (sizeof (ctype) * max_num_items);\
   if (list == NULL)\
     {\
	SLang_verror(SL_Malloc_Error, "Out of memory");\
	return;\
     }\
   dims[1] = dims[0] = 0;\
\
   while (SQLITE_ROW == sqlite3_step(ppStmt))\
     {\
	ctype i;\
	dims[0]++;\
	dims[1] = sqlite3_data_count(ppStmt);\
\
	for (i = 0; i < sqlite3_data_count (ppStmt); i++)\
	  {\
	     ctype value = sqlite3_column_##sqltype (ppStmt, i);\
	     if (max_num_items == num_items)\
	       {\
		  ctype *new_list;\
		  max_num_items += 4096;\
		  \
		  new_list = (ctype *) SLrealloc ((char *)list, sizeof (ctype) * max_num_items);\
		  if (new_list == NULL)\
		    {\
		       SLang_verror(SL_Malloc_Error, "Out of memory");\
		       goto return_error;\
		    }\
		  list = new_list;\
	       }\
	     \
	     list[num_items] = value;\
	     num_items++;\
	  }\
     }\
   if (num_items != max_num_items)\
     {\
	ctype *new_list = (ctype *)SLrealloc ((char *)list, sizeof (ctype) * (num_items + 1));\
	if (new_list == NULL)\
	  {\
	     SLang_verror(SL_Malloc_Error, "Out of memory");\
	     goto return_error;\
	  }\
	list = new_list;\
     }\
   if ((NULL == (at = SLang_create_array (slangtype, 0, (VOID_STAR) list, dims, 2)))\
       || (-1 == SLang_push_array (at, 1)))\
     SLang_push_null ();\
   return;\
   \
return_error:\
   SLfree ((char *)list);\
}

GET_ARRAY(integer, int, int, SLANG_INT_TYPE)
GET_ARRAY(double, double, double, SLANG_DOUBLE_TYPE)
#ifdef HAVE_LONG_LONG
GET_ARRAY(llong, long long, int64, SLANG_LLONG_TYPE)
#endif

static void sqlite_get_string_array(sqlite3_stmt *ppStmt)
{
   unsigned int num_items = 0, max_num_items = 1024;
   SLindex_Type dims[2];
   char **list;
   SLang_Array_Type *at;
   list = (char **) SLmalloc (sizeof (char *) * max_num_items);
   if (list == NULL)
     {
	SLang_verror(SL_Malloc_Error, "Out of memory");
	return;
     }

   dims[1] = dims[0] = 0;

   while (SQLITE_ROW == sqlite3_step(ppStmt))
     {
	int i;
	dims[0]++;
	dims[1] = sqlite3_data_count(ppStmt);

	for (i = 0; i < sqlite3_data_count (ppStmt); i++)
	  {
	     char * strp = (char *)sqlite3_column_text(ppStmt, i);
	     if (max_num_items == num_items)
	       {
		  char **new_list;
		  max_num_items += 4096;

		  new_list = (char **) SLrealloc ((char *)list, sizeof (char *) * max_num_items);
		  if (new_list == NULL)
		    {
		       SLang_verror(SL_Malloc_Error, "Out of memory");
		       goto return_error;
		    }
		  list = new_list;
	       }
	     strp = SLang_create_slstring(strp);

	     list[num_items] = strp;
	     num_items++;
	  }

     }

   if (num_items != max_num_items)
     {
	char **new_list = (char **)SLrealloc ((char *)list, sizeof (char *) * (num_items + 1));
	if (new_list == NULL)
	  {
	     SLang_verror(SL_Malloc_Error, "Out of memory");
	     goto return_error;
	  }
	list = new_list;
     }
   if ((NULL == (at = SLang_create_array (SLANG_STRING_TYPE, 0, (VOID_STAR) list, dims, 2)))
       || (-1 == SLang_push_array (at, 1)))
     SLang_push_null ();
   return;

return_error:
   while (num_items > 0)
     {
	num_items--;
	SLang_free_slstring (list[num_items]);
     }
   SLfree ((char *)list);
}

static void sqlite_get_bstring_array(sqlite3_stmt *ppStmt)
{
   unsigned int num_items = 0, max_num_items = 1024;
   SLindex_Type dims[2];
   SLang_BString_Type **list;
   SLang_Array_Type *at;
   SLang_BString_Type *bstr;

   list = (SLang_BString_Type **) SLmalloc (sizeof (SLang_BString_Type *) * max_num_items);
   if (list == NULL)
     {
	SLang_verror(SL_Malloc_Error, "Out of memory");
	return;
     }

   dims[1] = dims[0] = 0;

   while (SQLITE_ROW == sqlite3_step(ppStmt))
     {
	int i;
	dims[0]++;
	dims[1] = sqlite3_data_count(ppStmt);

	for (i = 0; i < sqlite3_data_count (ppStmt); i++)
	  {
	     if (max_num_items == num_items)
	       {
		  SLang_BString_Type **new_list;
		  max_num_items += 4096;

		  new_list = (SLang_BString_Type **) SLrealloc ((char *)list, sizeof (SLang_BString_Type *) * max_num_items);
		  if (new_list == NULL)
		    {
		       SLang_verror(SL_Malloc_Error, "Out of memory");
		       goto return_error;
		    }
		  list = new_list;
	       }
	     bstr = SLbstring_create((unsigned char *)sqlite3_column_blob(ppStmt, i),
				     sqlite3_column_bytes(ppStmt, i));

	     list[num_items] = bstr;
	     num_items++;
	  }

     }

   if (num_items != max_num_items)
     {
	SLang_BString_Type **new_list = (SLang_BString_Type **)SLrealloc ((char *)list, sizeof (SLang_BString_Type *) * (num_items + 1));
	if (new_list == NULL)
	  {
	     SLang_verror(SL_Malloc_Error, "Out of memory");
	     goto return_error;
	  }
	list = new_list;
     }
   if ((NULL == (at = SLang_create_array (SLANG_BSTRING_TYPE, 0, (VOID_STAR) list, dims, 2)))
       || (-1 == SLang_push_array (at, 1)))
     SLang_push_null ();
   return;

return_error:
   while (num_items > 0)
     {
	num_items--;
	SLbstring_free (list[num_items]);
     }
   SLfree ((char *)list);
}

static void slsqlite_get_array(void)
{
   db_type *p;
   SLang_MMT_Type *mmt;
   sqlite3_stmt *ppStmt;
   int nargs;
   char *sql;
   SLtype type;

   nargs = SLang_Num_Function_Args;

   if (nargs < 3)
     {
       SLdo_pop_n (nargs);
	SLang_verror(SL_Usage_Error, "usage: sqlite_get_array(Sqlite db, DataType type, String sql, ...)");
	return;
     }

   SLreverse_stack(nargs);

   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     {
	SLang_free_mmt (mmt);
	return;
     }
   p = (db_type *)SLang_object_from_mmt (mmt);

   nargs--;

   if (-1 == SLang_pop_datatype (&type))
     {
	SLang_verror(SL_Application_Error, "error in sqlite_get_array");
	SLang_free_mmt (mmt);
	SLdo_pop_n(nargs);
	return;
     }

   switch(type)
     {
      case SLANG_INT_TYPE:
#ifdef HAVE_LONG_LONG
      case SLANG_LLONG_TYPE:
#endif
      case SLANG_DOUBLE_TYPE:
      case SLANG_STRING_TYPE:
      case SLANG_BSTRING_TYPE:
	break;
      default:
	SLdo_pop_n (nargs - 1);
	SLang_verror(SL_Usage_Error, "only Integer, Double, String and Bstring types allowed");
	return;
     }

   if (-1 == SLang_pop_slstring(&sql))
     {
	SLang_verror(SL_Usage_Error, "usage: sqlite_get_array(type, Sqlite db, String sql, ...)");
	SLang_free_mmt(mmt);
	return;
     }

   if (check_error(p->db, sqlite3_prepare_v2(p->db, sql, -1, &ppStmt, NULL)))
     {
	goto free_return;
     }

   if (do_sqlite_bind(p->db, ppStmt, nargs - 2, 1))
     {
	sqlite3_finalize(ppStmt);
	goto free_return;
     }

   switch(type)
     {
      case SLANG_INT_TYPE:
	sqlite_get_integer_array(ppStmt); break;
      case SLANG_DOUBLE_TYPE:
	sqlite_get_double_array(ppStmt); break;
#ifdef HAVE_LONG_LONG
      case SLANG_LLONG_TYPE:
	sqlite_get_llong_array(ppStmt); break;
#endif
      case SLANG_STRING_TYPE:
	sqlite_get_string_array(ppStmt); break;
      case SLANG_BSTRING_TYPE:
	sqlite_get_bstring_array(ppStmt); break;
     }
free_return:
   check_error(p->db, sqlite3_finalize (ppStmt));

   SLang_free_slstring(sql);
   SLang_free_mmt(mmt);
}

/*}}}*/

static void slsqlite_exec(void)
{
   db_type *p;
   SLang_MMT_Type *mmt;
   sqlite3_stmt *ppStmt;
   int nargs;
   char *sql;

   nargs = SLang_Num_Function_Args;

   if (nargs < 2)
     {
       (void) SLdo_pop_n (nargs);
	SLang_verror(SL_Usage_Error, "usage: sqlite_exec(Sqlite db, String sql, ...)");
	return;
     }

   SLreverse_stack(nargs);
   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     return;

   p = (db_type *)SLang_object_from_mmt (mmt);

   if (-1 == SLang_pop_slstring(&sql))
     {
	SLang_verror(SL_Usage_Error, "usage: sqlite_get_row(Sqlite db, String sql, ...)");
	SLang_free_mmt(mmt);
	return;
     }

   if (check_error(p->db, sqlite3_prepare_v2(p->db, sql, -1, &ppStmt, NULL)))
     {
	goto free_return;
     }

   if (do_sqlite_bind(p->db, ppStmt, nargs - 2, 1))
     {
	sqlite3_finalize(ppStmt);
	goto free_return;
     }

   if (check_error(p->db, sqlite3_step(ppStmt)))
     sqlite3_finalize(ppStmt);
   else
     check_error(p->db, sqlite3_finalize (ppStmt));

free_return:
   SLang_free_slstring(sql);
   SLang_free_mmt(mmt);
}

static int slsqlite_changes (void)
{
   db_type *p;
   SLang_MMT_Type *mmt;
   int res;
   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     {
	SLang_free_mmt (mmt);
	return 0;
     }
   p = (db_type *)SLang_object_from_mmt (mmt);
   res = sqlite3_changes(p->db);
   SLang_free_mmt (mmt);
   return res;
}

static void slsqlite_bind_params (void)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;
   sqlite3 *db;
   int nargs;

   nargs = SLang_Num_Function_Args;

   if (nargs < 1)
     {
	SLdo_pop_n (nargs);
	SLang_verror(SL_Usage_Error, "usage: sqlite_bind_params(Statement stmt, ...)");
	return;
     }

   SLreverse_stack(nargs);

   if (NULL == (p = pop_statement (&mmt)))
     return;

   nargs--;

   /*
    * This is necessary because the documentation for sqlite_bind_* says
    *
    * SQLITE_MISUSE might be returned if these routines are called on a
    * virtual machine that is the wrong state or which has already been
    * finalized. [ ... ] Future versions of SQLite might panic rather than
    * return SQLITE_MISUSE.
    */
   if (p->state != SQLITE_OK)
     {
	SLang_verror (Sqlite_Error, "prepared statement is in wrong state (%d)", p->state);
	goto free_return;
     }

   db = sqlite3_db_handle(p->ppStmt);
   do_sqlite_bind(db, p->ppStmt, nargs, 1);
free_return:
   SLang_free_mmt(mmt);
}

/*
 * The next two function are needed to add support for named parameters in
 * the 'bind_params' method
 */

static void slsqlite_bind_param (void)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;
   sqlite3 *db;
   int nargs;
   int n;

   nargs = SLang_Num_Function_Args;

   if (nargs < 1)
     {
	SLdo_pop_n (nargs);
	SLang_verror(SL_Usage_Error, "usage: sqlite_bind_param(Statement stmt, int n, value)");
	return;
     }

   if (-1 == SLreverse_stack(nargs))
     return;

   if (NULL == (p = pop_statement (&mmt)))
     return;

   if (-1 == SLang_pop_int (&n))
     {
	SLang_free_mmt (mmt);
	return;
     }

   if (p->state != SQLITE_OK)
     {
	SLang_verror (Sqlite_Error, "prepared statement is in wrong state (%d)", p->state);
	goto free_return;
     }

   db = sqlite3_db_handle(p->ppStmt);
   do_sqlite_bind(db, p->ppStmt, n, n);
free_return:
   SLang_free_mmt(mmt);
}

static int slsqlite_bind_parameter_index (const char *name)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;
   int res = 0;

   if (NULL == (p = pop_statement (&mmt)))
     return -1;

   res = sqlite3_bind_parameter_index (p->ppStmt, name);
   SLang_free_mmt(mmt);
   return res;
}

static int slsqlite_step (void)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;
   int res = 0;

   if (NULL == (p = pop_statement (&mmt)))
     return -1;

   if (p->state != SQLITE_OK && p->state != SQLITE_ROW)
     {
	/*
	 * This can occur when trying to step in a ppStmt that has finished
	 */
	SLang_verror (Sqlite_Error, "prepared statement is in wrong state (%d)", p->state);
     }
   res = sqlite3_step (p->ppStmt);
   (void) check_error (sqlite3_db_handle (p->ppStmt), res);
   p->state = res;
   SLang_free_mmt (mmt);
   return res;
}

static void slsqlite_fetch(void)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;

   if (NULL == (p = pop_statement (&mmt)))
     return;

   if (p->state != SQLITE_ROW)
     {
	SLang_verror (Sqlite_Error, "prepared statement is in wrong state (%d)", p->state);
     }
   do_sqlite_fetch(p->ppStmt);
   SLang_free_mmt(mmt);
}

static void slsqlite_reset(void)
{
   Statement_Type *p;
   SLang_MMT_Type *mmt;

   if (NULL == (p = pop_statement (&mmt)))
     return;

   p->state = sqlite3_reset(p->ppStmt);

   SLang_free_mmt(mmt);
}

/*}}}*/
/*{{{ intrinsics */

static SLang_Intrin_Fun_Type Module_Intrinsics [] =
{
   MAKE_INTRINSIC_S("sqlite_open", slsqlite_open, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("sqlite_prepare", slsqlite_prepare, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("sqlite_get_table", slsqlite_get_table, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("sqlite_get_row", slsqlite_get_row, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("sqlite_get_array", slsqlite_get_array, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("sqlite_exec", slsqlite_exec, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("sqlite_changes", slsqlite_changes, SLANG_INT_TYPE),
   MAKE_INTRINSIC_0("sqlite_bind_params", slsqlite_bind_params, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("sqlite_bind_param", slsqlite_bind_param, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_S("sqlite_bind_parameter_index", slsqlite_bind_parameter_index, SLANG_INT_TYPE),
   MAKE_INTRINSIC_0("sqlite_step", slsqlite_step, SLANG_INT_TYPE),
   MAKE_INTRINSIC_0("sqlite_fetch", slsqlite_fetch, SLANG_VOID_TYPE),
   MAKE_INTRINSIC_0("sqlite_reset", slsqlite_reset, SLANG_VOID_TYPE),
   SLANG_END_INTRIN_FUN_TABLE
};

static SLang_Intrin_Var_Type Module_Variables [] =
{
   MAKE_VARIABLE("_sqlite_module_version_string", &Module_Version_String, SLANG_STRING_TYPE, 1),
   SLANG_END_INTRIN_VAR_TABLE
};

static SLang_IConstant_Type Module_Constants [] =
{
   MAKE_ICONSTANT("_sqlite_module_version", MODULE_VERSION_NUMBER),
   MAKE_ICONSTANT("SQLITE_OK", SQLITE_OK),
   MAKE_ICONSTANT("SQLITE_ERROR", SQLITE_ERROR),
   MAKE_ICONSTANT("SQLITE_INTERNAL", SQLITE_INTERNAL),
   MAKE_ICONSTANT("SQLITE_PERM", SQLITE_PERM),
   MAKE_ICONSTANT("SQLITE_ABORT", SQLITE_ABORT),
   MAKE_ICONSTANT("SQLITE_BUSY", SQLITE_BUSY),
   MAKE_ICONSTANT("SQLITE_LOCKED", SQLITE_LOCKED),
   MAKE_ICONSTANT("SQLITE_NOMEM", SQLITE_NOMEM),
   MAKE_ICONSTANT("SQLITE_READONLY", SQLITE_READONLY),
   MAKE_ICONSTANT("SQLITE_INTERRUPT", SQLITE_INTERRUPT),
   MAKE_ICONSTANT("SQLITE_IOERR", SQLITE_IOERR),
   MAKE_ICONSTANT("SQLITE_CORRUPT", SQLITE_CORRUPT),
   MAKE_ICONSTANT("SQLITE_NOTFOUND", SQLITE_NOTFOUND),
   MAKE_ICONSTANT("SQLITE_FULL", SQLITE_FULL),
   MAKE_ICONSTANT("SQLITE_CANTOPEN", SQLITE_CANTOPEN),
   MAKE_ICONSTANT("SQLITE_PROTOCOL", SQLITE_PROTOCOL),
   MAKE_ICONSTANT("SQLITE_EMPTY", SQLITE_EMPTY),
   MAKE_ICONSTANT("SQLITE_SCHEMA", SQLITE_SCHEMA),
   MAKE_ICONSTANT("SQLITE_TOOBIG", SQLITE_TOOBIG),
   MAKE_ICONSTANT("SQLITE_CONSTRAINT", SQLITE_CONSTRAINT),
   MAKE_ICONSTANT("SQLITE_MISMATCH", SQLITE_MISMATCH),
   MAKE_ICONSTANT("SQLITE_MISUSE", SQLITE_MISUSE),
   MAKE_ICONSTANT("SQLITE_NOLFS", SQLITE_NOLFS),
   MAKE_ICONSTANT("SQLITE_AUTH", SQLITE_AUTH),
   MAKE_ICONSTANT("SQLITE_FORMAT", SQLITE_FORMAT),
   MAKE_ICONSTANT("SQLITE_RANGE", SQLITE_RANGE),
   MAKE_ICONSTANT("SQLITE_NOTADB", SQLITE_NOTADB),
   MAKE_ICONSTANT("SQLITE_ROW", SQLITE_ROW),
   MAKE_ICONSTANT("SQLITE_DONE", SQLITE_DONE),
   SLANG_END_ICONST_TABLE
};

/*}}}*/
/*{{{ foreach */

struct _pSLang_Foreach_Context_Type
{
   SLang_MMT_Type *mmt;
   db_type *p;
   sqlite3_stmt *ppStmt;
};

static SLang_Foreach_Context_Type *cl_foreach_open (SLtype type, unsigned int num)
{
   SLang_Foreach_Context_Type *c = NULL;
   SLang_MMT_Type *mmt = NULL;
   char *s;

   (void) type;

   if (NULL == (mmt = SLang_pop_mmt (DB_Type_Id)))
     return NULL;

   if (!num)
     {
	SLang_verror (SL_Usage_Error, "Sqlite_Type requires an sql statement");
	SLang_free_mmt (mmt);
	return NULL;
     }

   SLreverse_stack(num);
   if (-1 == SLang_pop_slstring (&s))
     {
	SLang_verror (SL_Usage_Error, "Sqlite_Type requires an sql statement");
	SLang_free_mmt (mmt);
	return NULL;
     }

   if (NULL == (c = (SLang_Foreach_Context_Type *) SLmalloc (sizeof (SLang_Foreach_Context_Type))))
     goto free_return;

   memset ((char *) c, 0, sizeof (SLang_Foreach_Context_Type));

   c->mmt = mmt;
   c->p = (db_type *) SLang_object_from_mmt (mmt);
   if (check_error(c->p->db, sqlite3_prepare_v2(c->p->db, s, -1, &c->ppStmt, NULL)))
     goto free_return;

   if (do_sqlite_bind(c->p->db, c->ppStmt, num - 1, 1))
     {
	sqlite3_finalize(c->ppStmt);
	goto free_return;
     }

   return c;

free_return:
   SLang_free_slstring (s);
   SLang_free_mmt(mmt);
   return NULL;
}

static void cl_foreach_close (SLtype type, SLang_Foreach_Context_Type *c)
{
   (void) type;
   if (c == NULL) return;
   if (SQLITE_OK != sqlite3_finalize (c->ppStmt))
     SLang_verror (Sqlite_Error, "foreach_close failed");
   SLang_free_mmt (c->mmt);
   SLfree ((char *) c);
}

static int cl_foreach (SLtype type, SLang_Foreach_Context_Type *c)
{
   (void) type;

   if (c == NULL)
     return -1;
   return do_sqlite_step(c->p->db, c->ppStmt);
}

/*}}}*/
/*{{{ register class */

static void patchup_intrinsic_table (SLang_Intrin_Fun_Type *table,
				     unsigned char dummy, unsigned char type)
{
   while (table->name != NULL)
     {
	unsigned int i, nargs;
	SLtype *args;

	nargs = table->num_args;
	args = table->arg_types;
	for (i = 0; i < nargs; i++)
	  {
	     if (args[i] == dummy)
	       args[i] = type;
	  }

	/* For completeness */
	if (table->return_type == dummy)
	  table->return_type = type;

	table++;
     }
}

static int register_sqlite_type (void)
{
   SLang_Class_Type *cl;

   if (DB_Type_Id != 0)
     return 0;

   if (NULL == (cl = SLclass_allocate_class ("Sqlite_Type")))
     return -1;

   if (-1 == SLclass_set_destroy_function (cl, destroy_sqlite))
     return -1;

   if (-1 == SLclass_set_foreach_functions(cl, cl_foreach_open, cl_foreach, cl_foreach_close))
     return -1;

   /* By registering as SLANG_VOID_TYPE, slang will dynamically allocate a
    * type.
    */
   if (-1 == SLclass_register_class (cl, SLANG_VOID_TYPE, sizeof (db_type), SLANG_CLASS_TYPE_MMT))
     return -1;

   DB_Type_Id = SLclass_get_class_id (cl);

   patchup_intrinsic_table (Module_Intrinsics, DUMMY_SQLITE_TYPE, DB_Type_Id);

   return 0;
}

static int register_statement_type (void)
{
   SLang_Class_Type *cl;

   if (Statement_Type_Id != 0)
     return 0;

   if (NULL == (cl = SLclass_allocate_class ("Sqlite_Statement_Type")))
     return -1;

   if (-1 == SLclass_set_destroy_function (cl, destroy_statement))
     return -1;

   /* By registering as SLANG_VOID_TYPE, slang will dynamically allocate a
    * type.
    */
   if (-1 == SLclass_register_class (cl, SLANG_VOID_TYPE, sizeof (Statement_Type), SLANG_CLASS_TYPE_MMT))
     return -1;

   Statement_Type_Id = SLclass_get_class_id (cl);

   patchup_intrinsic_table (Module_Intrinsics, DUMMY_SQLITE_TYPE, Statement_Type_Id);

   return 0;
}

/*}}}*/
/*{{{ init */

int init_sqlite_module_ns (char *ns_name)
{
   SLang_NameSpace_Type *ns = SLns_create_namespace (ns_name);
   if (ns == NULL)
     return -1;
   if ((-1 == register_sqlite_type ())
       || (-1 == register_statement_type ()))
     return -1;

   if (Sqlite_Error == 0)
     {
	const Sqlite_Exception_Table_Type *b;
	b = Sqlite_Exception_Table;

	if (-1 == (Sqlite_Error = SLerr_new_exception (SL_RunTime_Error, "SqliteError", "Sqlite error")))
	  return -1;
	b++;
	while (b->errcode_ptr != NULL)
	  {
	     *b->errcode_ptr = SLerr_new_exception (Sqlite_Error, b->name, b->description);
	     if (*b->errcode_ptr == -1)
	       return -1;
	     b++;
	  }
     }

   if ((-1 == SLns_add_intrin_fun_table (ns, Module_Intrinsics, NULL))
       || (-1 == SLns_add_intrin_var_table (ns, Module_Variables, NULL))
       || (-1 == SLns_add_iconstant_table (ns, Module_Constants, NULL)))
     return -1;

   return 0;
}

/*}}}*/
